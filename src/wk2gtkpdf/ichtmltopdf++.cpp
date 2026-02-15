#include "ichtmltopdf++.h"
#include "iclog.h"
#include "index_pdf.h"

#include <X11/Xlib.h>
#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <gtk/gtk.h>
#include <iomanip>
#include <iostream>
#include <json-c/json.h>
#include <mutex>
#include <random>
#include <sstream>
#include <stdio.h>
#include <systemd/sd-bus.h>
#include <thread>
#include <wayland-client.h>
#include <webkit2/webkit2.h>

using std::string;

/**
 * @brief The WKGTK_init class
 *
 * Spawn thread with OUTER main loop - runs for lifetime of object
 */
WKGTK_init::WKGTK_init()
    : glob_Thread([this]() {
          loop = g_main_loop_new(nullptr, false);
          {
              std::lock_guard<std::mutex> lock(init_mutex);
          }
          init_cond.notify_one(); /**< Signal loop is ready */
          g_main_loop_run(loop);
      }) {
    // Wait for loop to be initialized
    std::unique_lock<std::mutex> lock(init_mutex);
    init_cond.wait(lock, [this] { return loop != nullptr; });
}

WKGTK_init::~WKGTK_init() {
    if (loop) {
        g_main_loop_quit(loop);
    }
    if (glob_Thread.joinable()) {
        glob_Thread.join();
    }
    if (loop) {
        g_main_loop_unref(loop);
    }
    jlog << iclog::loglevel::info << iclog::category::CORE
         << "GTK main loop exiting." << std::endl;
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
    : tk(handle_xvfb_daemon()),
      runMode(runMode) {
    jlog << iclog::loglevel::warning << iclog::category::CORE
         << "Inplicare initialising  WebKitGTK." << std::endl;
}

icGTK::~icGTK() {

    if (runMode == XvfbMode::START_STOP) {
        sd_bus *bus = nullptr;
        if (sd_bus_open_system(&bus) >= 0) {
            std::string state = check_xvfb(bus, "xvfb_2eservice");
            if (state == "active") {
                stop_service(bus);
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
WKGTK_init icGTK::handle_xvfb_daemon() {

    char *display  = getenv("DISPLAY");
    char *wayland  = getenv("WAYLAND_DISPLAY");
    bool  headless = false;
    if ((XOpenDisplay(display) == nullptr) && (wl_display_connect(wayland) == nullptr)) {
        headless = true;
        jlog << iclog::loglevel::info << iclog::category::CORE
             << "Preparing headless mode..." << std::endl;
    }

    sd_bus *bus = nullptr;

    if (headless) {
        if (sd_bus_open_system(&bus) < 0) {
            jlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
                 << "Failed to connect to system bus; "
                 << "the application cannot continue and will now exit" << std::endl;
            exit(1);
        }

        std::string unit  = "xvfb_2eservice"; // escaped 'xvfb.service'
        std::string state = check_xvfb(bus, unit);

        if (state != "active") {
            jlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
                 << unit << " not active, starting..." << std::endl;
            if (EXIT_SUCCESS == start_service(bus)) {
                jlog << iclog::loglevel::debug << iclog::category::CORE
                     << " state: " << check_xvfb(bus, unit) << std::endl;

                while (state != "active") {
                    state = check_xvfb(bus, unit);
                    jlog << iclog::loglevel::debug << iclog::category::CORE << unit
                         << " state: " << state << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
        }

        sd_bus_unref(bus);
        setenv("DISPLAY", ":99", 1);
    }

    if (gtk_init_check(NULL, NULL)) {
        jlog << iclog::loglevel::info << iclog::category::CORE
             << "WEBKIT2GTK Initialised." << std::endl;
    } else {
        jlog << iclog::loglevel::error << iclog::category::CORE
             << "GTK initialization failed; "
             << "the application cannot continue and will now exit." << std::endl;
        exit(1);
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
std::string icGTK::check_xvfb(sd_bus *bus, const std::string &service) {

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
        if (strcmp(error.name, "org.freedesktop.DBus.Error.UnknownObject") == 0) {
            std::cerr << "Service does not exist\n";
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
bool icGTK::start_service(sd_bus *bus) {
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
        jlog << iclog::loglevel::error << iclog::category::CORE
             << "Failed to start unit: " << error.message << std::endl;
    } else {
        // Read the returned job path
        r = sd_bus_message_read(reply, "o", &job_path);
        if (r >= 0) {
            jlog << iclog::loglevel::debug << iclog::category::CORE
                 << "Started job: " << job_path << std::endl;
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
bool icGTK::stop_service(sd_bus *bus) {
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
        jlog << iclog::loglevel::error << iclog::category::CORE
             << "Failed to Stop unit: " << error.message << std::endl;
    } else {
        // Read the returned job path
        r = sd_bus_message_read(reply, "o", &job_path);
        if (r >= 0) {
            jlog << iclog::loglevel::debug << iclog::category::CORE
                 << "Stopped job: " << job_path << std::endl;
        }
    }
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    return (r < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

struct PDFprinterUserData {
        GtkPrintSettings                *print_settings;
        WebKitPrintOperation            *print_operation;
        GMainLoop                       *main_loop;
        std::vector<PDFprinter::anchor> *linkData;
        int                             *tocPage;
        index_mode                       doIndex;
};

/**
 * @brief print_finished
 * @param print_operation
 * @param user_data
 *
 * Print finshed callback.
 */
static void print_finished(WebKitPrintOperation *print_operation __attribute__((unused)), void *user_data) {
    g_main_loop_quit(((PDFprinterUserData *)user_data)->main_loop);
    jlog << iclog::loglevel::debug << iclog::category::CORE
         << "Printing complte; quitting." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief INDEXABLE PDF
 */

/**
 * @brief js_code
 *
 * This is javascript to generate JSON that eventullly be used to overlay the pdf
 * with anchor points and references as webkit2gtk cannot do this natively.
 *
 *
 */

// --- EXPERIMENTAL CODE START ---
// Added JavaScript extraction for index and anchor positions
// This code is experimental and may need refinement
const char *js_code_classic =
    "window.indexPositions = []; "
    "window.targetData = {}; "
    "window.tocPage; "

    "let pages = document.querySelectorAll('.page'); "
    "   for (let i = 0; i < pages.length; i++) { "
    "       if (pages[i].hasAttribute(\"toc\")) { "
    "           window.tocPage = { page: i }; "
    "           break; "
    "       } "
    "   } "

    "function getPageNumber(element) { "
    "    let current = element; "
    "    while (current) { "
    "        if (current.classList && current.classList.contains('page')) { "
    "            const allPages = document.querySelectorAll('.page'); "
    "            for (let i = 0; i < allPages.length; i++) { "
    "                if (allPages[i] === current) return i + 1; "
    "            } "
    "        } "
    "        current = current.parentElement; "
    "    } "
    "    return 0; "
    "} "

    "function getClosestPageElement(element) { "
    "    let current = element; "
    "    while (current) { "
    "        if (current.classList && current.classList.contains('page')) { "
    "            return current; "
    "        } "
    "        current = current.parentElement; "
    "    } "
    "    return null; "
    "} "

    // "document.querySelectorAll('.index-item').forEach(item => { "
    // "    const link = item.querySelector('a'); "
    // "    const href = link.getAttribute('href'); "
    // "    const id = href.substring(1); "
    // "    const rect = item.getBoundingClientRect(); "
    // "    const pageElement = getClosestPageElement(item); "
    // "    const pageRect = pageElement.getBoundingClientRect(); "
    // "    const x = rect.left - pageRect.left; "
    // "    const y = rect.top - pageRect.top; "
    // "    window.indexPositions[id] = { "
    // "        x: x, "
    // "        y: y, "
    // "        width: rect.width, "
    // "        height: rect.height, "
    // "        page: getPageNumber(item), "
    // "        page_width: pageRect.width, "
    // "        page_height: pageRect.height "
    // "    }; "
    // "}); "

    "document.querySelectorAll('a').forEach(item => { "
    "    if (item.hasAttribute('href'))  { "
    "        const href = item.getAttribute('href'); "
    "        if (href.charAt(0) === '#') { "
    "            const id = href.substring(1); "
    "            const rect = item.getBoundingClientRect(); "
    "            const pageElement = getClosestPageElement(item); "
    "            if (!pageElement) { "
    "                return; "
    "            } "
    "            const pageRect = pageElement.getBoundingClientRect(); "
    "            const x = rect.left - pageRect.left; "
    "            const y = rect.top - pageRect.top; "
    "            window.indexPositions.push ({ "
    "                id: id, "
    "                x: x, "
    "                y: y, "
    "                width: rect.width, "
    "                height: rect.height, "
    "                page: getPageNumber(item), "
    "                page_width: pageRect.width, "
    "                page_height: pageRect.height "
    "            }); "
    "        }; "
    "    }; "
    "}); "

    "document.querySelectorAll('[id]').forEach(elt => { "
    "    const id = elt.getAttribute('id'); "
    "    const rect = elt.getBoundingClientRect(); "
    "    const pageElement = getClosestPageElement(elt); "
    "    if (!pageElement) { "
    "        return; "
    "    } "
    "    const pageRect = pageElement.getBoundingClientRect(); "
    "    const x = rect.left - pageRect.left; "
    "    const y = rect.top - pageRect.top; "
    "    window.targetData[id] = { "
    "        title: elt.innerText, "
    "        x: x, "
    "        y: y, "
    "        width: rect.width, "
    "        height: rect.height, "
    "        page: getPageNumber(elt), "
    "        page_width: pageRect.width, "
    "        page_height: pageRect.height "
    "    }; "
    "}); "

    "JSON.stringify({ "
    "    toc: window.tocPage, "
    "    indexPositions: window.indexPositions, "
    "    targetData: window.targetData "
    "});";

const char *js_code_enhanced =
    "window.indexPositions = []; "
    "window.targetData = {}; "
    "window.tocPage; "

    "let pages = document.querySelectorAll('.page'); "
    "   for (let i = 0; i < pages.length; i++) { "
    "       if (pages[i].hasAttribute(\"toc\")) { "
    "           window.tocPage = { page: i }; "
    "           break; "
    "       } "
    "   } "

    "function getPageNumber(element) { "
    "    let current = element; "
    "    while (current) { "
    "        if (current.classList && current.classList.contains('page')) { "
    "            const allPages = document.querySelectorAll('.page'); "
    "            for (let i = 0; i < allPages.length; i++) { "
    "                if (allPages[i] === current) return i + 1; "
    "            } "
    "        } "
    "        current = current.parentElement; "
    "    } "
    "    return 0; "
    "} "

    "function getClosestPageElement(element) { "
    "    let current = element; "
    "    while (current) { "
    "        if (current.classList && current.classList.contains('page')) { "
    "            return current; "
    "        } "
    "        current = current.parentElement; "
    "    } "
    "    return null; "
    "} "

    "document.querySelectorAll('.index-item').forEach(item => { "
    "    const link = item.querySelector('a'); "
    "    if(link) { "
    "        if (link.hasAttribute('href'))  { "
    "            const href = link.getAttribute('href'); "
    "            const id = href.substring(1); "
    "            const rect = item.getBoundingClientRect(); "
    "            const pageElement = getClosestPageElement(item); "
    "                if (!pageElement) { "
    "                    return; "
    "                } "
    "            const pageRect = pageElement.getBoundingClientRect(); "
    "            const x = rect.left - pageRect.left; "
    "            const y = rect.top - pageRect.top; "
    "            window.indexPositions.push ({ "
    "                id: id, "
    "                x: x, "
    "                y: y, "
    "                width: rect.width, "
    "                height: rect.height, "
    "                page: getPageNumber(item), "
    "                page_width: pageRect.width, "
    "                page_height: pageRect.height "
    "            }); "
    "        }; "
    "    }; "
    "}); "

    "document.querySelectorAll('[id]').forEach(elt => { "
    "    const id = elt.getAttribute('id'); "
    "    const rect = elt.getBoundingClientRect(); "
    "    const pageElement = getClosestPageElement(elt); "
    "    if (!pageElement) { "
    "        return; "
    "    } "
    "    const pageRect = pageElement.getBoundingClientRect(); "
    "    const x = rect.left - pageRect.left; "
    "    const y = rect.top - pageRect.top; "
    "    window.targetData[id] = { "
    "        title: elt.innerText, "
    "        x: x, "
    "        y: y, "
    "        width: rect.width, "
    "        height: rect.height, "
    "        page: getPageNumber(elt), "
    "        page_width: pageRect.width, "
    "        page_height: pageRect.height "
    "    }; "
    "}); "

    "JSON.stringify({ "
    "    toc: window.tocPage, "
    "    indexPositions: window.indexPositions, "
    "    targetData: window.targetData "
    "});";

PDFprinter::anchor &get_anchor(std::vector<PDFprinter::anchor> &linkData, std::string key) {
    for (PDFprinter::anchor &it : linkData) {
        if (it.linkName.compare(key) == 0) {
            return (it);
        }
    }
    throw std::out_of_range("Key not found in linkData; this may not be an error.");
}

/**
 * @brief javascript_callback
 * @param web_view
 * @param result
 * @param user_data
 *
 * This is a callback used to grab the physical coordinates of of the
 * anchors so that we can use them to create links.
 */
static void javascript_callback(
    WebKitWebView *web_view,
    GAsyncResult  *result,
    gpointer       user_data
) {
    GError *error = NULL;

    JSCValue *js_result = webkit_web_view_evaluate_javascript_finish(
        web_view,
        result,
        &error
    );

    if (error) {
        jlog << iclog::loglevel::error << iclog::category::CORE
             << "JavaScript error: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // Convert JSCValue to string
    gchar *json_string = jsc_value_to_string(js_result);

    if (!json_string) {
        jlog << iclog::loglevel::error << iclog::category::CORE
             << "Failed to convert JavaScript result to string" << std::endl;
        g_object_unref(js_result);
        return;
    }

    jlog << iclog::loglevel::debug << iclog::category::CORE
         << "Extracted JSON: " << json_string << std::endl;

    // Parse JSON with json-c
    json_object *root = json_tokener_parse(json_string);
    if (!root) {
        jlog << iclog::loglevel::error << iclog::category::CORE
             << "Failed to parse JSON" << std::endl;
        g_free(json_string);
        g_object_unref(js_result);
        return;
    }

    json_object *toc_obj, *page_obj;
    if (json_object_object_get_ex(root, "toc", &toc_obj)) {
        if (json_object_object_get_ex(toc_obj, "page", &page_obj)) {
            (*((PDFprinterUserData *)user_data)->tocPage) = json_object_get_int(page_obj);
        }
    }

    // Extract indexPositions (the <a> tags)
    // json_object                     *indexPositions = json_object_object_get(root, "indexPositions");
    // std::vector<PDFprinter::anchor> &linkData       = *((PDFprinterUserData *)user_data)->linkData;
    // if (indexPositions) {
    //     json_object_object_foreach(indexPositions, key, val) {
    //         double x      = json_object_get_double(json_object_object_get(val, "x"));
    //         double y      = json_object_get_double(json_object_object_get(val, "y"));
    //         double width  = json_object_get_double(json_object_object_get(val, "width"));
    //         double height = json_object_get_double(json_object_object_get(val, "height"));
    //         double pageW  = json_object_get_int(json_object_object_get(val, "page_width"));
    //         double pageH  = json_object_get_int(json_object_object_get(val, "page_height"));
    //         int    page   = json_object_get_int(json_object_object_get(val, "page"));
    //         linkData.push_back({
    //             key,
    //             {string(), x,    y,    width, height, pageW, pageH, page},
    //             {string(), 0.0f, 0.0f, 0.0f,  0.0f,   0.0f,  0.0f,  0   }
    //         });

    //         jlog << iclog::loglevel::debug << iclog::category::CORE
    //              << "Finding Index: " << key << " -> page: " << page
    //              << " pos: (" << x << "," << y << ")" << std::endl;
    //     }
    // }
    // NEW JSON ITERATOR FOR SOURCE (Index) POSITIONS
    json_object                     *indexPositions = json_object_object_get(root, "indexPositions");
    std::vector<PDFprinter::anchor> &linkData       = *((PDFprinterUserData *)user_data)->linkData;

    if (indexPositions && json_object_get_type(indexPositions) == json_type_array) {
        size_t len = json_object_array_length(indexPositions);
        for (size_t i = 0; i < len; ++i) {
            json_object *val = json_object_array_get_idx(indexPositions, i);

            const char *id     = json_object_get_string(json_object_object_get(val, "id"));
            double      x      = json_object_get_double(json_object_object_get(val, "x"));
            double      y      = json_object_get_double(json_object_object_get(val, "y"));
            double      width  = json_object_get_double(json_object_object_get(val, "width"));
            double      height = json_object_get_double(json_object_object_get(val, "height"));
            double      pageW  = json_object_get_double(json_object_object_get(val, "page_width"));
            double      pageH  = json_object_get_double(json_object_object_get(val, "page_height"));
            int         page   = json_object_get_int(json_object_object_get(val, "page"));

            linkData.push_back({
                id ? std::string(id) : std::string(),
                {std::string(), x,    y,    width, height, pageW, pageH, page},
                {std::string(), 0.0f, 0.0f, 0.0f,  0.0f,   0.0f,  0.0f,  0   }
            });

            jlog << iclog::loglevel::debug << iclog::category::CORE
                 << "Finding Index: " << (id ? id : "null") << " -> page: " << page
                 << " pos: (" << x << "," << y << ")" << std::endl;
        }
    }
    // END

    // Extract targetData (the target id)
    json_object *targets = json_object_object_get(root, "targetData");

    std::vector<std::pair<std::string, PDFprinter::linkData>> tData;
    if (targets) {
        json_object_object_foreach(targets, key, val) {
            try {

                tData.push_back({
                    key,
                    {json_object_get_string(json_object_object_get(val, "title")),
                      json_object_get_double(json_object_object_get(val, "x")),
                      json_object_get_double(json_object_object_get(val, "y")),
                      json_object_get_double(json_object_object_get(val, "width")),
                      json_object_get_double(json_object_object_get(val, "height")),
                      json_object_get_double(json_object_object_get(val, "page_width")),
                      json_object_get_double(json_object_object_get(val, "page_height")),
                      json_object_get_int(json_object_object_get(val, "page"))}
                });

            } catch (std::out_of_range e) {
                jlog << iclog::loglevel::warning << iclog::category::LIB
                     << e.what()
                     << std::endl;
            }
        }
    }

    for (PDFprinter::anchor &s : linkData) {
        for (std::pair<std::string, PDFprinter::linkData> t : tData) {
            if (s.linkName.compare(t.first) == 0) {
                s.target = t.second;
                jlog << iclog::loglevel::debug << iclog::category::CORE
                     << "\nIndex: " << s.linkName << " -> page: " << s.index.pageNo << " pos: (" << s.index.xPos << "," << s.index.yPos << ") size: (" << s.index.w << "," << s.index.h << ")\n"
                     << "Target: " << s.linkName << " -> title: " << s.target.title << " -> page: " << s.target.pageNo << " pos: (" << s.target.xPos << "," << s.target.yPos << ") size: (" << s.target.w << "," << s.target.h << ")"
                     << std::endl;
            }
        }
    }

    // Cleanup
    json_object_put(root);
    g_free(json_string);
    g_object_unref(js_result);

    // After extraction, trigger print
    WebKitPrintOperation *print_operation = ((PDFprinterUserData *)user_data)->print_operation;
    webkit_print_operation_print(print_operation);
}

// --- EXPERIMENTAL CODE END ---

/**
 * @brief web_view_load_changed
 * @param web_view
 * @param load_event
 * @param user_data
 *
 * Web load monitor callback
 */
static void web_view_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, void *user_data) {

    switch (load_event) {
        case WEBKIT_LOAD_STARTED:
            jlog << iclog::loglevel::debug << iclog::category::CORE
                 << "WEBKIT LOAD STARTED." << std::endl;
            /* New load, we have now a provisional URI */
            // printf("WEBKIT_LOAD_STARTED\n");

            break;
        case WEBKIT_LOAD_REDIRECTED:
            break;
        case WEBKIT_LOAD_COMMITTED:
            jlog << iclog::loglevel::debug << iclog::category::CORE
                 << "The load is being performed. Current URI is the final one and it "
                    "won't change unless a new "
                 << "load is requested or a navigation within the same page is "
                    "performed."
                 << std::endl;
            break;

            // --- EXPERIMENTAL CODE START ---
            // Added JavaScript extraction for index and anchor positions
            // This code is experimental and may need refinement

            // --- REMOVED ---
            // case WEBKIT_LOAD_FINISHED:
            //     jlog << iclog::loglevel::debug << iclog::category::CORE
            //          << "printing pdf file: "
            //          << gtk_print_settings_get(
            //                 ((PDFprinterUserData *)user_data)->print_settings,
            //                 GTK_PRINT_SETTINGS_OUTPUT_URI
            //             )
            //          << std::endl;
            //     {

            //         WebKitPrintOperation *print_operation = ((PDFprinterUserData *)user_data)->print_operation;
            //         webkit_print_operation_print(print_operation);
            //     }
            //     break;

            // --- ADDED ---
        case WEBKIT_LOAD_FINISHED:
            jlog << iclog::loglevel::debug << iclog::category::CORE
                 << "WEBKIT LOAD FINISHED - extracting positions" << std::endl;

            auto js = [&user_data]() {
                return (
                    ((PDFprinterUserData *)user_data)->doIndex
                            == index_mode::ENHANCED
                        ? js_code_enhanced
                        : js_code_classic
                );
            };

            // Check if we need to extract positions (i.e., index generation)
            if (
                (((PDFprinterUserData *)user_data)->doIndex == index_mode::ENHANCED)
                || (((PDFprinterUserData *)user_data)->doIndex == index_mode::CLASSIC)
            ) {
                // Enable JavaScript for extraction
                WebKitSettings *view_settings = webkit_web_view_get_settings(web_view);
                webkit_settings_set_enable_javascript(view_settings, true);
                jlog << iclog::loglevel::debug << iclog::category::CORE
                     << "Extracting coordinates using:\n"
                     << js()
                     << std::endl;
                // Evaluate JS to extract positions
                webkit_web_view_evaluate_javascript(
                    web_view,
                    js(), // script
                    -1,   // length (use -1 for null-terminated string)
                    NULL, // world_name
                    NULL, // source_uri
                    NULL, // cancellable
                    (GAsyncReadyCallback)javascript_callback,
                    user_data
                );

            } else {
                // No extraction needed — proceed directly to print
                jlog << iclog::loglevel::debug << iclog::category::CORE
                     << "No index extraction required — printing directly" << std::endl;

                WebKitPrintOperation *print_operation = ((PDFprinterUserData *)user_data)->print_operation;
                webkit_print_operation_print(print_operation);
            }
            break;
            // --- EXPERIMENTAL CODE END ---
    }
}

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief cb_worker
 * @param p
 * @return
 *
 * This is a callback for Webkit2GTK that actually generates the PDF
 *
 * @note It is **NOT** threadsafe as Webkit2GTK is event driven and handles
 * calls in a queue.
 */
static int cb_worker(struct html2pdf_params *p) {
    struct PDFprinterUserData user_data;
    user_data.linkData = (std::vector<PDFprinter::anchor> *)p->indexData;
    user_data.doIndex  = p->doIndex;
    user_data.tocPage  = p->tocPage;

    jlog << iclog::loglevel::debug << iclog::category::CORE
         << "Applying print settings" << std::endl;
    GtkPrintSettings *print_settings = gtk_print_settings_new();
    gtk_print_settings_set_printer(print_settings, "Print to File");
    gtk_print_settings_set(print_settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");

    GtkPageSetup *page_setup = gtk_page_setup_new();

    if (p->key_file_data != NULL) {

        jlog << iclog::loglevel::debug << iclog::category::CORE
             << "Applying page setup:\n"
             << p->key_file_data << std::endl;
        GKeyFile *key_file = g_key_file_new();
        g_key_file_load_from_data(key_file, p->key_file_data, (gsize)-1, G_KEY_FILE_NONE, NULL);
        gtk_page_setup_load_key_file(page_setup, key_file, NULL, NULL);
        gtk_print_settings_load_key_file(print_settings, key_file, NULL, NULL);

        g_key_file_free(key_file);
    }

    gtk_print_settings_set(print_settings, GTK_PRINT_SETTINGS_OUTPUT_URI, p->out_uri);

    user_data.print_settings = print_settings;

    WebKitWebContext         *web_context          = webkit_web_context_new_ephemeral();
    WebKitWebView            *web_view             = 0;
    WebKitUserContentManager *user_content_manager = 0;
    WebKitUserStyleSheet     *user_stylesheet      = 0;

    web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(web_context));

    WebKitSettings *view_settings = webkit_web_view_get_settings(web_view);
    webkit_settings_set_enable_javascript(view_settings, false);
    webkit_settings_set_enable_page_cache(view_settings, false);
    webkit_settings_set_enable_html5_database(view_settings, false);
    webkit_settings_set_enable_html5_local_storage(view_settings, false);

    if (p->default_stylesheet) {

        jlog << iclog::loglevel::debug << iclog::category::CORE
             << "Injecting style sheet:\n"
             << p->default_stylesheet << std::endl;
        user_content_manager = webkit_user_content_manager_new();
        user_stylesheet      = webkit_user_style_sheet_new(
            p->default_stylesheet,
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_STYLE_LEVEL_USER,
            NULL,
            NULL
        );
        webkit_user_content_manager_add_style_sheet(user_content_manager, user_stylesheet);
        g_object_set_property(G_OBJECT(web_view), "user-content-manager", (GValue *)(user_content_manager));
    } else {
        jlog << iclog::loglevel::debug << iclog::category::CORE
             << "Injecting stylesheet: No explicit style sheet set; skipping:"
             << std::endl;
    }

    g_object_ref_sink(G_OBJECT(web_view));

    // WebKit2GTK print is async - schedules work and returns immediately
    WebKitPrintOperation *print_operation = webkit_print_operation_new(web_view);
    webkit_print_operation_set_print_settings(print_operation, print_settings);
    webkit_print_operation_set_page_setup(print_operation, page_setup);
    g_signal_connect(print_operation, "finished", G_CALLBACK(print_finished), &user_data);
    user_data.print_operation = print_operation;

    // INNER LOOP: Created here, runs until WebKit2GTK print completes
    GMainLoop *main_loop = g_main_loop_new(nullptr, false);
    user_data.main_loop  = main_loop;
    g_signal_connect(web_view, "load-changed", G_CALLBACK(web_view_load_changed), &user_data);
    if (p->in_uri == NULL) {
        // webkit_web_view_load_html(web_view, p->html_txt, "file:///tmp");
        webkit_web_view_load_html(web_view, p->html_txt, "file:///");
        // webkit_web_view_load_html(web_view, p->html_txt, p->base_uri);
    } else {
        webkit_web_view_load_uri(web_view, p->in_uri);
    }

    // Run inner loop until print callback signals completion
    g_main_loop_run(main_loop);

    g_object_unref(G_OBJECT(print_operation));
    g_object_unref(G_OBJECT(print_settings));
    g_object_unref(G_OBJECT(page_setup));

    if (p->default_stylesheet) {
        g_object_unref(G_OBJECT(user_content_manager));
        webkit_user_style_sheet_unref(user_stylesheet);
    }

    gtk_widget_destroy(GTK_WIDGET(web_view));
    g_object_unref(G_OBJECT(web_view));
    g_object_unref(G_OBJECT(web_context));
    g_main_loop_unref(main_loop);

    std::mutex              *wait_mutex = p->wait_mutex;
    std::condition_variable *wait_cond  = p->wait_cond;
    int                     *wait_data  = p->wait_data;

    if (wait_mutex && wait_cond && wait_data) {
        {
            std::lock_guard<std::mutex> lock(*wait_mutex);
            (*wait_data)++;
        }
        // Signal outer loop that we're done
        wait_cond->notify_one(); // or notify_all()
    }

    return G_SOURCE_REMOVE;
}

/**
 * @brief PDFprinter::PDFprinter
 */
PDFprinter::PDFprinter() {
    in_uri             = nullptr;
    html_txt           = nullptr;
    base_uri           = nullptr;
    out_uri            = nullptr;
    key_file_data      = nullptr;
    default_stylesheet = nullptr;
    m_makeBlob         = false;
    m_doIndex          = index_mode::OFF;
    m_tocPage          = index_pdf::UNSET;
}

/**
 * @brief PDFprinter::~PDFprinter
 */
PDFprinter::~PDFprinter() {

    if (in_uri != nullptr)
        delete[] in_uri;
    if (html_txt != nullptr)
        delete[] html_txt;
    if (base_uri != nullptr)
        delete[] base_uri;
    if (out_uri != nullptr)
        delete[] out_uri;
    if (key_file_data != nullptr)
        delete[] key_file_data;
    if (default_stylesheet != nullptr)
        delete[] default_stylesheet;
}

/**
 * @brief PDFprinter::read_file
 * @param fullPath
 * @return The contents of the file
 *
 * This is just a utility funciton that takes in a path to a file and
 * returns its contents.
 */
std::string PDFprinter::read_file(const std::string &fullPath) {
    std::ifstream file(fullPath);
    if (file.fail()) {
        jlog << iclog::loglevel::debug << iclog::category::CORE
             << "Cannot find the file specified: " << fullPath << std::endl;
        return ("");
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void PDFprinter::to_cstring(const std::string &str, char *&cstr) {
    delete[] cstr;
    size_t size = str.size() + 1; /**< +1 for null terminator */
    cstr        = new char[size];
    std::copy(str.data(), str.data() + str.size(), cstr);
    cstr[str.size()] = '\0';
};

/**
 * @brief PDFprinter::read_file_to_blob
 *
 * If the caller wishes to conduct post processing then we return a
 * blob rather than a file.
 *
 * This method creates the Blob and then deletes the file.
 *
 */
void PDFprinter::read_file_to_blob() {

    string path(out_uri);
    path = path.substr(sizeof("file://") - 1, path.size());

    std::ifstream file(path, std::ios::binary | std::ios::ate | std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open PDF file: " + string(path));
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    m_binPDF.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char *>(m_binPDF.data()), size);

    if (!file) {
        throw std::runtime_error("Error reading PDF file contents");
    }

    jlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
         << "Generated BLOB: " << path << " size=" << m_binPDF.size() << std::endl;
}

std::string PDFprinter::generate_uuid_string() {
    static std::mt19937             gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, 255);

    std::vector<unsigned> sets{4, 2, 2, 2, 6};
    std::stringstream     ss;
    ss << std::hex << std::setfill('0');

    for (size_t i = 0; i < sets.size(); ++i) {
        for (unsigned j = 0; j < sets[i]; ++j) {
            ss << std::setw(2) << dis(gen);
        }
        if (i < sets.size() - 1) {
            ss << "-";
        }
    }

    std::string uuid = ss.str();
    jlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
         << "UUID generated: " << uuid << std::endl;

    return uuid;
}

/**
 * @brief Configure PDF generation with HTML, print settings, and output file.
 *
 * @param html Raw HTML content (not a file path).
 * @param printSettings GTK print settings string (keyfile format).
 * @param outFile Output file path where the PDF will be written.
 *
 * This is the primary method for generating PDFs with custom print settings.
 *
 * @note Internal C-strings are allocated by a lambda and automatically
 * cleaned up in the destructor.
 *
 * @see layout() for custom page setup without print settings.
 */
void PDFprinter::set_param(std::string html, std::string printSettings, std::string outFile, index_mode createIndex) {
    m_doIndex = createIndex;
    to_cstring(html, html_txt);
    m_destFile = outFile;
    to_cstring(printSettings, key_file_data);
}

/**
 * @brief Configure PDF generation with HTML and output file (no print settings).
 *
 * @param html Raw HTML content (not a file path).
 * @param outFile Output file path where the PDF will be written.
 *
 * Use this method when you need custom page setup via layout() instead of
 * providing explicit print settings.
 *
 * @see layout() to configure page size, margins, and orientation.
 * @see set_param(std::string, std::string, std::string) for print settings.
 */
void PDFprinter::set_param(std::string html, std::string outFile, index_mode createIndex) {
    m_doIndex  = createIndex;
    m_destFile = outFile;
    to_cstring(html, html_txt);
}

/**
 * @brief Configure PDF generation with HTML only (output as BLOB).
 *
 * @param html Raw HTML content (not a file path).
 *
 * Use this method when you want to retrieve the PDF as a binary blob
 * instead of writing to a file. This allows post-processing (e.g., with
 * ImageMagick) without intermediate file I/O.
 *
 * @note layout() must be called to configure page setup before generating
 * the PDF.
 *
 * @return Call make_pdf() to create the PDF, then get_blob() to
 * retrieve the binary data.
 *
 * @see layout() to configure page setup.
 * @see make_pdf() to create the PDF.
 * @see get_blob() to retrieve the PDF as a binary blob.
 */
void PDFprinter::set_param(std::string html, index_mode createIndex) {
    m_doIndex = createIndex;
    to_cstring(html, html_txt);
    m_makeBlob = true;
}

void PDFprinter::layout(std::string pageSize, std::string orientation) {

    struct PaperSize sz{"A4", 210, 297};

    for (const PaperSize &it : isoPaperSizes) {
        if (it.sizeName.compare(pageSize) == 0) {
            sz = it;
            break;
        }
    }

    // Make lower case
    std::transform(orientation.begin(), orientation.end(), orientation.begin(), [](unsigned char c) { return std::tolower(c); });
    string o = "portrait";
    if (orientation.compare("landscape") == 0)
        o = orientation;

    std::string printSettings(
        /* clang-format off */
        "[Print Settings]\n"
        "quality=high\n"
        "resolution=300\n"
        "output-file-format=pdf\n"
        "printer=Print to File\n"
        "page-set=all\n"
        "[Page Setup]\n"
        "PPDName=" + sz.sizeName + "\n"
        "DisplayName=" + sz.sizeName + "\n"
        "Width=" + std::to_string(sz.shortMM) + "\n"
        "Height=" + std::to_string(sz.longMM) + "\n"
        "MarginTop=0\n"
        "MarginBottom=0\n"
        "MarginLeft=0\n"
        "MarginRight=0\n"
        "Orientation=" + o + "\n"
        /* clang-format on */
    );

    to_cstring(printSettings, key_file_data);
}

/**
 * @brief PDFprinter::make_pdf
 *
 * Generate the pdf.
 */
void PDFprinter::make_pdf() {

    // DIRECTLY CREATE THE PDF
    if ((m_doIndex == index_mode::OFF) && !m_destFile.empty())
        to_cstring("file://" + m_destFile, out_uri);

    std::string tempFile = "/tmp/" + generate_uuid_string();

    // POST PROCESS (index or create blob)
    if ((m_doIndex != index_mode::OFF) || m_makeBlob)
        to_cstring("file://" + tempFile, out_uri);

    // MAKE THE PDF
    std::thread t([this]() {
        std::mutex              wait_mutex;
        std::condition_variable wait_cond;
        int                     wait_data = 0;

        payload.out_uri            = out_uri;
        payload.html_txt           = html_txt;
        payload.key_file_data      = key_file_data;
        payload.in_uri             = nullptr;
        payload.base_uri           = nullptr;
        payload.default_stylesheet = nullptr;
        payload.wait_cond          = &wait_cond;
        payload.wait_mutex         = &wait_mutex;
        payload.wait_data          = &wait_data;
        payload.indexData          = &m_indexData;
        payload.doIndex            = m_doIndex;
        payload.tocPage            = &m_tocPage;

        g_idle_add((GSourceFunc)cb_worker, &payload);

        {
            std::unique_lock<std::mutex> lock(wait_mutex);
            wait_cond.wait(lock, [&wait_data] { return wait_data != 0; });
        }
    });

    t.join();

    // CREATE INDEX (if requested)
    if ((m_doIndex == index_mode::CLASSIC) || (m_doIndex == index_mode::ENHANCED)) {

        index_pdf idx(m_indexData, m_tocPage);
        idx.create_anchors(tempFile, m_destFile);
        std::remove(tempFile.c_str());
    }

    // GENERATE BLOB (if requested)
    if (m_makeBlob) {
        jlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
             << "Making BLOB" << std::endl;
        read_file_to_blob();
    }
}

const PDFprinter::blob &PDFprinter::get_blob() const { return m_binPDF; }

PDFprinter::blob &&PDFprinter::get_blob() { return (std::move(m_binPDF)); }

std::vector<PDFprinter::anchor> &&PDFprinter::get_anchor_data() { return (std::move(m_indexData)); }
