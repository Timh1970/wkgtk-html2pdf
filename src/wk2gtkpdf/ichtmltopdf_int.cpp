#include "ichtmltopdf_int.h"

#include "iclog.h"

#include <X11/Xlib.h>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <systemd/sd-bus.h>
#include <thread>
#include <wayland-client.h>
#include <webkit2/webkit2.h>

struct WKGTK_init_impl {
        GMainLoop              *glob_loop = nullptr;
        std::thread             glob_Thread;
        std::mutex              init_mutex;
        std::condition_variable init_cond;
};

WKGTK_init::WKGTK_init()
    : m_pimpl(new WKGTK_init_impl()) {

    WKGTK_init_impl *p   = m_pimpl;
    m_pimpl->glob_Thread = std::thread([p]() {
        p->glob_loop = g_main_loop_new(nullptr, false);
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

struct icGTK_impl {
        XvfbMode    runMode;
        WKGTK_init *tk = nullptr; // Allocated on the heap to hide its size

        std::string check_xvfb(sd_bus *bus, const std::string &service);
        WKGTK_init  handle_xvfb_daemon();
        bool        start_service(sd_bus *bus);
        bool        stop_service(sd_bus *bus);
};

/**
 * @brief icGTK::init
 * @return
 * avoid linker errors.
 */
icGTK &icGTK::init() {
    return icGTK::init(XvfbMode::KEEP_RUNNING);
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
icGTK::icGTK(XvfbMode runMode)
    : m_pimpl(new icGTK_impl()) {
    m_pimpl->tk      = new WKGTK_init(m_pimpl->handle_xvfb_daemon());
    m_pimpl->runMode = runMode;
    wkJlog << iclog::loglevel::info << iclog::category::CORE
           << "Inplicare initialising  WebKitGTK."
           << iclog::endl;
}

WKGTK_init::WKGTK_init(WKGTK_init &&other) noexcept
    : m_pimpl(other.m_pimpl) {
    other.m_pimpl = nullptr; // Null out the old one so its destructor does nothing
}

icGTK::~icGTK() {

    if (m_pimpl->runMode == XvfbMode::START_STOP) {
        sd_bus *bus = nullptr;
        if (sd_bus_open_system(&bus) >= 0) {
            std::string state = m_pimpl->check_xvfb(bus, "xvfb_2eservice");
            if (state == "active") {
                m_pimpl->stop_service(bus);
            }
            sd_bus_unref(bus);
        }
    }
}

/**
 * @brief icGTK_init::getInstance
 * @param runMode
 * @return single instance
 *
 * This uses the Myers singleton approach to ensure we can only call
 * the class once.  It is genius, but I cannot claim credit for it.
 */
icGTK &icGTK::init(XvfbMode runMode) {
    static icGTK instance(runMode);
    return instance;
}

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

    char *display  = getenv("DISPLAY");
    char *wayland  = getenv("WAYLAND_DISPLAY");
    bool  headless = false;

    bool has_x11     = (display && display[0] != '\0');
    bool has_wayland = (wayland && wayland[0] != '\0');

    if ((!has_x11 || XOpenDisplay(display) == nullptr) && (!has_wayland || wl_display_connect(wayland) == nullptr)) {
        headless = true;
        wkJlog << iclog::loglevel::info << iclog::category::CORE
               << "No valid display found (X11/Wayland). Preparing headless mode..." << iclog::endl;
    }

    sd_bus *bus = nullptr;

    if (headless) {

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
        setenv("DISPLAY", ":99", 1);
    }

    if (gtk_init_check(NULL, NULL)) {
        wkJlog << iclog::loglevel::info << iclog::category::CORE
               << "WEBKIT2GTK Initialised." << iclog::endl;
    } else {
        wkJlog << iclog::loglevel::error << iclog::category::CORE
               << "GTK initialization failed"
               << iclog::endl;
        throw std::runtime_error("GTK initialization failed");
    }

    return (WKGTK_init());
}

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
