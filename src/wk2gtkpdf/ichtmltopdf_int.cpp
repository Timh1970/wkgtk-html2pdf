#include "ichtmltopdf_int.h"

#include "iclog.h"

#include <X11/Xlib.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#ifdef USE_SYSTEMD
#include <string>
#include <systemd/sd-bus.h>
#else
#include <cstring>
#include <libdinitctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#endif

#include <thread>
#include <wayland-client.h>
#ifdef USE_WEBKIT_6
#include <webkit/webkit.h>
#else
#include <webkit2/webkit2.h>
#endif

WKGTKRunMode phtml::WKGTK_run_mode = WKGTKRunMode::UNSET;

/**************************************/
/*                                    */
/*                                    */
/*  WKGTK_init PIMPL                  */
/*                                    */
/*                                    */
/**************************************/
struct WKGTK_init_impl {
        GMainLoop              *glob_loop = nullptr;
        std::thread             glob_Thread;
        std::mutex              init_mutex;
        std::condition_variable init_cond;

        static gboolean silence_recent_files(gpointer);
};

gboolean WKGTK_init_impl::silence_recent_files(gpointer) {
    GtkSettings *settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings, "gtk-recent-files-enabled", FALSE, NULL);
    }
    return G_SOURCE_REMOVE; // Only run once
}

WKGTK_init::WKGTK_init()
    : m_pimpl(new WKGTK_init_impl()) {
    WKGTK_init_impl *p   = m_pimpl;
    m_pimpl->glob_Thread = std::thread([p]() {
        p->glob_loop = g_main_loop_new(nullptr, false);
        g_idle_add(p->silence_recent_files, nullptr);

        {
            std::lock_guard<std::mutex> lock(p->init_mutex);
            // ... any other setup ...
        }
        p->init_cond.notify_one();
        // This is the GTK heart; it needs 'p' to stay valid
        g_main_loop_run(p->glob_loop);
    });

    std::unique_lock<std::mutex> lock(m_pimpl->init_mutex);
    m_pimpl->init_cond.wait(lock, [this] { return m_pimpl->glob_loop != nullptr; });
}

WKGTK_init::~WKGTK_init() {
    // 1. THE GOLDEN RULE: If we were moved, m_pimpl is null. Exit immediately.
    if (!m_pimpl)
        return;

    // 2. Stop the loop FIRST
    if (m_pimpl->glob_loop) {
        g_main_loop_quit(m_pimpl->glob_loop);
    }

    // 3. Join the thread SECOND (Wait for g_main_loop_run to actually exit)
    if (m_pimpl->glob_Thread.joinable()) {
        m_pimpl->glob_Thread.join();
    }

    // 4. Cleanup the GLib memory THIRD
    if (m_pimpl->glob_loop) {
        g_main_loop_unref(m_pimpl->glob_loop);
    }

    // 5. Delete the Brain LAST
    delete m_pimpl;
    m_pimpl = nullptr;
    wkJlog << iclog::loglevel::info << iclog::category::CORE
           << "GTK Global loop exiting." << iclog::endl;
}

/**************************************/
/*                                    */
/*                                    */
/*  icGTK_impl PIMPL                  */
/*                                    */
/*                                    */
/**************************************/
struct icGTK_impl {
        WKGTK_init      *tk      = nullptr; // Allocated on the heap to hide its size
        std::atomic_bool gui_run = false;
#ifdef USE_SYSTEMD
        std::string check_xvfb(sd_bus *bus, const std::string &service);
        bool        start_service(sd_bus *bus);
        bool        stop_service(sd_bus *bus);
#else
        bool check_xvfb_socket();
        bool dinit_wake_service(const std::string &service_name);
        bool dinit_is_service_running(const std::string &service_name);
        bool dinit_stop_service(const std::string &service_name);
        bool ensure_xvfb_is_ready();

#endif
        WKGTK_init handle_xvfb_daemon();
};

/**
 * @brief icGTK::init
 * @return
 * avoid linker errors.
 */
icGTK &icGTK::init() {
    return icGTK::init(WKGTKRunMode::KEEP_RUNNING);
}

/**
 * @brief pdf_init::pdf_init
 * @param runMode - defaults to KEEP_RUNNING
 *
 * Check if the xvfb daemon is required and if so start it then
 * initialise webktGTK.
 *
 * @note START_STOP run mode is primarily used for testing.
 *
 */
icGTK::icGTK(WKGTKRunMode runMode)
    : m_pimpl(new icGTK_impl()) {
    m_pimpl->tk           = new WKGTK_init(m_pimpl->handle_xvfb_daemon());
    phtml::WKGTK_run_mode = runMode;
}

WKGTK_init::WKGTK_init(WKGTK_init &&other) noexcept
    : m_pimpl(other.m_pimpl) {
    other.m_pimpl = nullptr; // Null out the old one so its destructor does nothing
}

icGTK::~icGTK() {
#ifdef USE_SYSTEMD
    if (phtml::WKGTK_run_mode == WKGTKRunMode::START_STOP) {
        sd_bus *bus = nullptr;
        if (sd_bus_open_system(&bus) >= 0) {
            std::string state = m_pimpl->check_xvfb(bus, "xvfb_2eservice");
            if (state == "active") {
                m_pimpl->stop_service(bus);
            }
            sd_bus_unref(bus);
        }
    }
#else
    wkJlog << iclog::loglevel::info << iclog::category::CORE
           << "Exiting application context."
           << iclog::endl;
#endif
}

/**
 * @brief icGTK_init::getInstance
 * @param runMode
 * @return single instance
 *
 * This uses the Myers singleton approach to ensure we can only call
 * the class once.  It is genius, but I cannot claim credit for it.
 */
icGTK &icGTK::init(WKGTKRunMode runMode) {
    static icGTK instance(runMode);
    return instance;
}

#ifndef USE_SYSTEMD
bool icGTK_impl::check_xvfb_socket() {
    return (access("/tmp/.X11-unix/X99", F_OK) == 0);
}
#endif

/**
 * @brief pdf_init::handle_xvfb_daemon
 * @return - A new instance of WKGTK_init
 *
 * Only start the xvfb daemon if necessary.
 *
 * @note  If for some reason starting webkit2GTK fails then
 * this will exit the application entirely
 */
WKGTK_init icGTK_impl::handle_xvfb_daemon() {

    char *display           = getenv("DISPLAY");
    char *wayland           = getenv("WAYLAND_DISPLAY");
    bool  display_connected = false;

    bool has_x11     = (display && display[0] != '\0');
    bool has_wayland = (wayland && wayland[0] != '\0');

    // 1. Try X11
    if (has_x11) {
        Display *x_disp = XOpenDisplay(display);
        if (x_disp) {
            display_connected = true;
            XCloseDisplay(x_disp);
        }
    }

    // 2. Try Wayland (if X11 failed)
    if (!display_connected && has_wayland) {
        struct wl_display *w_disp = wl_display_connect(wayland);
        if (w_disp) {
            display_connected = true;
            wl_display_disconnect(w_disp);
        }
    }

    if (!display_connected) {
        wkJlog << iclog::loglevel::info << iclog::category::CORE
               << "No valid display found (X11/Wayland). Preparing headless mode..." << iclog::endl;
#ifdef USE_SYSTEMD
        sd_bus *bus = nullptr;

        int r = sd_bus_open_system(&bus);
        if (r < 0) {
            wkJlog << iclog::loglevel::error << "D-Bus Connection Failed: " << strerror(-r) << iclog::endl;
            throw std::runtime_error("Failed to connect to system bus: " + std::string(strerror(-r)));
        }

        std::string unit  = "xvfb_2eservice"; // escaped 'xvfb.service'
        std::string state = check_xvfb(bus, unit);

        if (state != "active") {
            wkJlog << iclog::loglevel::info << iclog::category::CORE << iclog_FUNCTION
                   << unit << " not active, starting..." << iclog::endl;
            if (EXIT_SUCCESS == start_service(bus)) {
                wkJlog << iclog::loglevel::debug << iclog::category::CORE
                       << " state: " << check_xvfb(bus, unit) << iclog::endl;

                while (state != "active") {
                    state = check_xvfb(bus, unit);
                    wkJlog << iclog::loglevel::debug << iclog::category::CORE << unit
                           << " state: " << state << iclog::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
        }

        sd_bus_unref(bus);
#else
        wkJlog << iclog::loglevel::info << iclog::category::CORE
               << "Probing high-availability virtual display filesystem socket..." << iclog::endl;

        // Perform a fast filesystem probe for the active shared frame buffer
        if (access("/tmp/.X11-unix/X99", F_OK) != 0) {
            wkJlog << iclog::loglevel::notice << iclog::category::CORE
                   << "High-Availability display :99 cold. Activating via dinit socket..." << iclog::endl;

            // Direct socket invocation without shelling out or spawning threads
            if (!dinit_wake_service("xvfb")) {
                wkJlog << iclog::loglevel::error << "Dinit IPC transaction failed. Check socket write access." << iclog::endl;
                throw std::runtime_error("Headless initialization failed: Unable to communicate with dinit.");
            }

            // High-Availability Safeguard: Wait for the hardware file socket to warm up
            int retries = 15;
            while (access("/tmp/.X11-unix/X99", F_OK) != 0 && retries > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                retries--;
            }

            if (access("/tmp/.X11-unix/X99", F_OK) != 0) {
                throw std::runtime_error("Dinit activated xvfb, but display socket failed to initialize.");
            }
        }
#endif

        setenv("DISPLAY", ":99", 1);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Disabling desktop sessions." << iclog::endl;
#ifdef USE_WEBKIT_6
        gtk_disable_portals();
#endif
        // TRY AND PREVENT PRINT CALLBACK WAITING FOR A DESKTOP ENVORONMENT ON DEBIAN
        g_setenv("GIO_USE_VFS", "local", TRUE);
        g_setenv("NO_AT_BRIDGE", "1", TRUE);
        g_setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null", TRUE);
    }

    g_set_prgname("ichtmltopdf");

    // SHOULD NOT BE NEEDED
    // g_setenv("G_DBUS_CONFIG_FILE", "/dev/null", TRUE);
    // FORCE GTK to ignore portals globally for this process.
    // This must happen BEFORE the first GTK/WebKit call in this thread.
    g_setenv("GTK_USE_PORTAL", "0", TRUE);
    g_setenv("GDK_DEBUG", "no-portals", TRUE);
    // WebKit often ignores GTK_USE_PORTAL for proxy/network settings
    g_setenv("XDG_PROXIES_DISABLED", "1", TRUE);
#ifdef USE_WEBKIT_6

    // 1. WebKit6 / GTK4:
    // force the Sandbox OFF so it can see the Xvfb display socket.
    // setenv("WEBKIT_FORCE_SANDBOX", "0", 1);

    // // 2. Disable the Compositor and Hardware Acceleration for Xvfb.
    // // This stops the EGL/DRI2 warnings from becoming fatal Trace Traps.
    // setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 1);

    // setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);

    // // 3. Fake a D-Bus address if one isn't present to stop the "dbus-launch" error.
    // if (!getenv("DBUS_SESSION_BUS_ADDRESS")) {
    //     setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null", 0);
    // }

    // GTK4: No arguments, returns boolean
    if (gtk_init_check()) {
        wkJlog << iclog::loglevel::info << "WEBKIT 6 (GTK4) Initialised." << iclog::endl;
    } else {
        wkJlog << iclog::loglevel::error << "GTK4 initialization failed" << iclog::endl;
        throw std::runtime_error("GTK4 initialization failed");
    }
#else
    // GTK3: Needs NULL pointers for argc/argv compatibility
    if (gtk_init_check(NULL, NULL)) {
        wkJlog << iclog::loglevel::info << "WEBKIT 2 (GTK3) Initialised." << iclog::endl;
    } else {
        wkJlog << iclog::loglevel::error << "GTK3 initialization failed" << iclog::endl;
        throw std::runtime_error("GTK3 initialization failed");
    }
#endif

    return (WKGTK_init());
}

#ifdef USE_SYSTEMD
/**************************************/
/*                                    */
/*                                    */
/*  SYSTEMD METHODS                   */
/*                                    */
/*                                    */
/**************************************/
/**
 * @brief icGTK::check_xvfb
 * @param bus
 * @param service
 * @return the current state string
 *
 * Check the status of the xvfb service and return one of the following:
 *
 * active:          Unit is running.
 * reloading:       Unit is active and reloading configuration.
 * inactive:        Unit is stopped, but last run was successful or never
 * started. failed: Unit is stopped, and the last run failed.
 * activating:      Unit is in the process of starting.
 * deactivating:    Unit is in the process of stopping
 */
std::string icGTK_impl::check_xvfb(sd_bus *bus, const std::string &service) {

    sd_bus_error error = SD_BUS_ERROR_NULL;
    char        *state = nullptr;
    int          r     = sd_bus_get_property_string(
        bus,
        "org.freedesktop.systemd1",
        ("/org/freedesktop/systemd1/unit/" + service).c_str(),
        "org.freedesktop.systemd1.Unit",
        "ActiveState",
        nullptr,
        &state
    );
    if (r < 0) {
        if (error.name != nullptr) {
            if (strcmp(error.name, "org.freedesktop.DBus.Error.UnknownObject") == 0) {
                wkJlog << iclog::loglevel::error << iclog::category::CORE
                       << "Service does not exist"
                       << iclog::endl;
            }
        }
        return "";
    }
    sd_bus_error_free(&error);
    std::string result(state);
    free(state);
    return result;
}

/**
 * @brief pdf_init::start_service
 * @param bus
 * @return
 *
 * Start the xfvb service.
 */
bool icGTK_impl::start_service(sd_bus *bus) {
    sd_bus_error    error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    const char     *job_path;
    int             r = sd_bus_call_method(
        bus,
        "org.freedesktop.systemd1",         /**< service */
        "/org/freedesktop/systemd1",        /**< object path */
        "org.freedesktop.systemd1.Manager", /**< interface */
        "StartUnit",                        /**< method */
        &error,                             /**< error object */
        &reply,                             /**< response object */
        "ss",                               /**< we are passing 2 strings (the service name and "replace" */
        "xvfb.service",                     /**< unit name */
        "replace"                           /**< mode */
    );

    if (r < 0) {
        wkJlog << iclog::loglevel::error << iclog::category::CORE
               << "Failed to start unit: " << error.message << iclog::endl;
    } else {
        // Read the returned job path
        r = sd_bus_message_read(reply, "o", &job_path);
        if (r >= 0) {
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Started job: " << job_path << iclog::endl;
        }
    }
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    return (r < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**
 * @brief pdf_init::stop_service
 * @param bus
 * @return
 *
 * Stop the xfvb service.
 *
 * This is conditionally called in the destructor if
 * START_STOP mode is selected.
 *
 * @warning START_STOP mode has the potential to be error prone
 * in environments where mutiple applications may need the process.
 *
 * It is only recommended to use for testing rather than produciton
 * environments where xvfb should be left running.
 */
bool icGTK_impl::stop_service(sd_bus *bus) {
    sd_bus_error    error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    const char     *job_path;
    int             r = sd_bus_call_method(
        bus,
        "org.freedesktop.systemd1",         /**< service */
        "/org/freedesktop/systemd1",        /**< object path */
        "org.freedesktop.systemd1.Manager", /**< interface */
        "StopUnit",                         /**< method */
        &error,                             /**< error object */
        &reply,                             /**< response object */
        "ss",                               /**< we are passing 2 strings (the service name and "replace" */
        "xvfb.service",                     /**< unit name */
        "replace"                           /**< mode */
    );

    if (r < 0) {
        wkJlog << iclog::loglevel::error << iclog::category::CORE
               << "Failed to Stop unit: " << error.message << iclog::endl;
    } else {
        // Read the returned job path
        r = sd_bus_message_read(reply, "o", &job_path);
        if (r >= 0) {
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Stopped job: " << job_path << iclog::endl;
        }
    }
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    return (r < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
#endif

/**************************************/
/*                                    */
/*                                    */
/*  DINIT METHODS                     */
/*                                    */
/*                                    */
/**************************************/
/**
 * @brief icGTK_impl::dinit_wake_service
 * @param service_name - The target dinit service to wake up
 * @return true if the service command is accepted successfully
 */
bool icGTK_impl::dinit_wake_service(const std::string &service_name) {
    dinitctl *ctl = dinitctl_open_system();
    if (!ctl) {
        return false;
    }

    dinitctl_service_handle *handle = nullptr;

    dinitctl_service_state current_state;
    dinitctl_service_state target_state;

    int load_res = dinitctl_load_service(ctl, service_name.c_str(), false, &handle, &current_state, &target_state);
    if (load_res < 0) {
        dinitctl_close(ctl);
        return false;
    }

    int start_res = dinitctl_start_service(ctl, handle, false);

    dinitctl_close(ctl);

    return (start_res >= 0);
}

/**
 * @brief icGTK_impl::dinit_is_service_running
 * @param service_name - The service to inspect (e.g., "xvfb")
 * @return true if the service is currently fully running (STARTED)
 */
bool icGTK_impl::dinit_is_service_running(const std::string &service_name) {
    dinitctl *ctl = dinitctl_open_system();
    if (!ctl) {
        return false;
    }

    dinitctl_service_handle *handle        = nullptr;
    dinitctl_service_state   current_state = DINITCTL_SERVICE_STATE_STOPPED;
    dinitctl_service_state   target_state  = DINITCTL_SERVICE_STATE_STOPPED;

    // Load the service data. This fills 'current_state' with the real status
    int load_res = dinitctl_load_service(ctl, service_name.c_str(), false, &handle, &current_state, &target_state);

    dinitctl_close(ctl);

    if (load_res < 0) {
        return false;
    }

    return (current_state == DINITCTL_SERVICE_STATE_STARTED);
}

/**
 * @brief icGTK_impl::dinit_stop_service
 * @param service_name - The target dinit service to stop
 * @return true if the service teardown signal is sent successfully
 */
bool icGTK_impl::dinit_stop_service(const std::string &service_name) {
    dinitctl *ctl = dinitctl_open_system();
    if (!ctl) {
        return false;
    }

    dinitctl_service_handle *handle = nullptr;

    // FIXED: Use the specific enum type requested by libdinitctl.h
    dinitctl_service_state current_state;
    dinitctl_service_state target_state;

    if (dinitctl_load_service(ctl, service_name.c_str(), false, &handle, &current_state, &target_state) < 0) {
        dinitctl_close(ctl);
        return false;
    }

    // Execute stop command
    int stop_res = dinitctl_stop_service(ctl, handle, 0, false, false);

    dinitctl_close(ctl);

    return (stop_res >= 0);
}

bool icGTK_impl::ensure_xvfb_is_ready() {
    // 1. Fire the start trigger
    if (!dinit_wake_service("xvfb")) {
        return false;
    }

    // 2. Poll the status until the state moves past STARTING to STARTED
    int retry_limit = 20; // 20 * 50ms = 1 second max timeout
    while (retry_limit > 0) {
        if (dinit_is_service_running("xvfb")) {
            return true; // The virtual framebuffer is active! WebKit can render now.
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        retry_limit--;
    }

    return false; // Timed out waiting for Xvfb to settle
}
