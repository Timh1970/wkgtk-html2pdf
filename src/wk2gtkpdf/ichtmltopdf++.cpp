#include "ichtmltopdf++.h"
#include "iclog.h"
#include "index_pdf.h"

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <gtk/gtk.h>
#include <iomanip>
#include <iostream>
#include <json-c/json.h>
#include <mutex>
#include <random>
#include <sstream>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>

#ifdef USE_WEBKIT_6
#include <webkit/webkit.h>
#define WK_EXTERN_PRINT_SETTINGS GtkPrintSettings
#else
#include <webkit2/webkit2.h>
#define WK_EXTERN_PRINT_SETTINGS WebKitPrintSettings
#endif

extern "C" {
const char *wk2gtkpdf_version() {
    return APP_VERSION; // unknown
}
}

/**
 * @brief js_code
 *
 * This is javascript to generate JSON that eventullly be used to overlay the pdf
 * with anchor points and references as webkit2gtk cannot do this natively.
 *
 *
 */
namespace phtml {

    const char *js_code_classic =
        "window.indexPositions = []; "
        "window.targetData = {}; "
        "window.tocPage; "

        "let pages = document.querySelectorAll('.page'); "
        "for (let i = 0; i < pages.length; i++) { "
        "    if (pages[i].hasAttribute(\"toc\")) { "
        "        window.tocPage = { page: i }; "
        "        break; "
        "    } "
        "} "

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
        "        if (current.classList && current.classList.contains('page')) return current; "
        "        current = current.parentElement; "
        "    } "
        "    return null; "
        "} "

        "document.querySelectorAll('a').forEach(item => { "
        "    if (item.hasAttribute('href')) { "
        "        const href = item.getAttribute('href'); "
        "        if (href.charAt(0) === '#') { "
        "            const id = href.substring(1); "
        "            const pageElement = getClosestPageElement(item); "
        "            if (!pageElement) return; "
        "            const pageRect = pageElement.getBoundingClientRect(); "
        "            "
        "            /* Fix for multi-line classic anchors */ "
        "            const rects = item.getClientRects(); "
        "            for (let i = 0; i < rects.length; i++) { "
        "                const r = rects[i]; "
        "                window.indexPositions.push({ "
        "                    id: id, "
        "                    x: r.left - pageRect.left, "
        "                    y: r.top - pageRect.top, "
        "                    width: r.width, "
        "                    height: r.height, "
        "                    page: getPageNumber(item), "
        "                    page_width: pageRect.width, "
        "                    page_height: pageRect.height "
        "                }); "
        "            } "
        "        } "
        "    } "
        "}); "

        "document.querySelectorAll('[id]').forEach(elt => { "
        "    const id = elt.getAttribute('id'); "
        "    const rect = elt.getBoundingClientRect(); "
        "    const pageElement = getClosestPageElement(elt); "
        "    if (!pageElement) return; "
        "    const pageRect = pageElement.getBoundingClientRect(); "
        "    window.targetData[id] = { "
        "        title: elt.innerText, "
        "        x: rect.left - pageRect.left, "
        "        y: rect.top - pageRect.top, "
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
        "for (let i = 0; i < pages.length; i++) { "
        "    if (pages[i].hasAttribute(\"toc\")) { "
        "        window.tocPage = { page: i }; "
        "        break; "
        "    } "
        "} "

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
        "        if (current.classList && current.classList.contains('page')) return current; "
        "        current = current.parentElement; "
        "    } "
        "    return null; "
        "} "

        "document.querySelectorAll('.index-item').forEach(item => { "
        "    const link = item.querySelector('a'); "
        "    if(link && link.hasAttribute('href')) { "
        "        const id = link.getAttribute('href').substring(1); "
        "        /* SURGERY: Map the ROW fragments, not the LINK text */ "
        "        const rects = item.getClientRects(); "
        "        for (let i = 0; i < rects.length; i++) { "
        "            const r = rects[i]; "
        "            const target = document.elementFromPoint(r.left + 2, r.top + 2); "
        "            const actualPage = getClosestPageElement(target) || getClosestPageElement(item); "
        "            if (!actualPage) continue; "
        "            const pageRect = actualPage.getBoundingClientRect(); "
        "            window.indexPositions.push({ "
        "                id: id, "
        "                x: r.left - pageRect.left, "
        "                y: r.top - pageRect.top, "
        "                width: r.width, "
        "                height: r.height, "
        "                page: getPageNumber(actualPage), "
        "                page_width: pageRect.width, "
        "                page_height: pageRect.height "
        "            }); "
        "        } "
        "    } "
        "}); "

        "document.querySelectorAll('[id]').forEach(elt => { "
        "    const id = elt.getAttribute('id'); "
        "    const rect = elt.getBoundingClientRect(); "
        "    const pageElement = getClosestPageElement(elt); "
        "    if (!pageElement) return; "
        "    const pageRect = pageElement.getBoundingClientRect(); "
        "    window.targetData[id] = { "
        "        title: elt.innerText, "
        "        x: rect.left - pageRect.left, "
        "        y: rect.top - pageRect.top, "
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

    /**
     * @brief isoPaperSizes
     * - This is a list of standard well known page
     * sizes (they are not all ISO)
     * - @todo remove this and maintain a hidden JSON
     * file in the /usr/share/wk2gtkpdf to retain
     * the template data.
     * - Parse that file instead of this struct to
     * allow users to add custom paper sizes using
     * the template_maker utility.
     **/
    static const PaperSize isoPaperSizes[52]{
        {"A0",      841.0222,  1188.8611},
        {"A1",      593.7250,  841.0222 },
        {"A2",      419.8056,  593.7250 },
        {"A3",      296.6861,  419.8056 },
        {"A4",      209.9028,  296.6861 }, // The magic 296.68 (841pt)
        {"A5",      147.8139,  209.9028 },
        {"A6",      104.7750,  147.8139 },
        {"A7",      73.7306,   104.7750 },
        {"A8",      51.8583,   73.7306  },
        {"A9",      36.6889,   51.8583  },
        {"A10",     25.7528,   36.6889  },
        {"SRA0",    899.9361,  1279.8778},
        {"SRA1",    639.9389,  899.9361 },
        {"SRA2",    449.7917,  639.9389 },
        {"SRA3",    319.9694,  449.7917 },
        {"SRA4",    224.7194,  319.9694 },
        {"B0",      1000.1250, 1413.9306},
        {"B1",      706.9667,  1000.1250},
        {"B2",      499.8861,  706.9667 },
        {"B3",      352.7778,  499.8861 },
        {"B4",      249.7667,  352.7778 },
        {"B5",      175.6833,  249.7667 },
        {"B6",      124.8833,  175.6833 },
        {"B7",      87.8417,   124.8833 },
        {"B8",      61.7361,   87.8417  },
        {"B9",      43.7444,   61.7361  },
        {"B10",     30.6917,   43.7444  },
        {"C0",      916.8694,  1296.8111},
        {"C1",      647.7000,  916.8694 },
        {"C2",      457.9056,  647.7000 },
        {"C3",      323.8500,  457.9056 },
        {"C4",      228.9528,  323.8500 },
        {"C5",      161.9250,  228.9528 },
        {"C6",      113.9472,  161.9250 },
        {"C7",      80.7861,   113.9472 },
        {"C8",      56.7972,   80.7861  },
        {"C9",      39.8639,   56.7972  },
        {"C10",     27.8694,   39.8639  },
        {"ANSIA",   215.9000,  279.4000 }, // Perfect 612x792pt
        {"ANSIB",   279.4000,  431.8000 },
        {"ANSIC",   431.8000,  558.8000 },
        {"ANSID",   558.8000,  863.6000 },
        {"ANSIE",   863.6000,  1117.6000},
        {"Letter",  215.9000,  279.4000 }, // 11 inches exactly
        {"Legal",   215.9000,  355.6000 }, // 14 inches exactly
        {"Tabloid", 279.4000,  431.8000 }, // 17 inches exactly
        {"ArchA",   228.6000,  304.8000 },
        {"ArchB",   304.8000,  457.2000 },
        {"ArchC",   457.2000,  609.6000 },
        {"ArchD",   609.6000,  914.4000 },
        {"ArchE",   914.4000,  1219.2000},
        {nullptr,   0.0,       0.0      }
    };

    /******************************************************************************/
    /*                                                                            */
    /*                                                                            */
    /*  PDFprinter PIMPL pointer                                                  */
    /*                                                                            */
    /*                                                                            */
    /******************************************************************************/
    struct PDFprinter_impl {

            char                    *in_uri             = nullptr;
            char                    *html_txt           = nullptr;
            char                    *base_uri           = nullptr;
            char                    *out_uri            = nullptr;
            char                    *key_file_data      = nullptr;
            char                    *default_stylesheet = nullptr;
            bool                     m_makeBlob         = false;
            index_mode               m_doIndex          = index_mode::OFF;
            int                      m_tocPage          = index_pdf::UNSET;
            std::mutex              *wait_mutex         = nullptr;
            std::condition_variable *wait_cond          = nullptr;
            int                     *wait_data          = nullptr;
            WebKitWebView           *m_web_view         = nullptr;
            GtkPrintSettings        *m_print_settings   = nullptr;
            WebKitPrintOperation    *m_print_operation  = nullptr;
            GMainLoop               *m_innerLoop        = nullptr;
            char                    *m_destFile         = nullptr;

            /**
             * @brief m_processing
             *
             * This flag is used explicitly in GUI mode only, in normal
             * mode the thread and mutexes handle state.
             */
            bool                       m_processing = false;
            std::vector<unsigned char> m_binPDF;

            PDF_Anchor *m_indexData         = nullptr;
            size_t      m_indexDataCount    = 0;
            size_t      m_indexDataCapacity = 0;

            ~PDFprinter_impl() {
                wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
                       << "Cleaning up class object."
                       << iclog::endl;

                delete[] in_uri;
                delete[] html_txt;
                delete[] base_uri;
                delete[] out_uri;
                delete[] key_file_data;
                delete[] default_stylesheet;
                delete[] m_destFile;
                wait_cond  = nullptr;
                wait_mutex = nullptr;
                wait_data  = nullptr;
                wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
                       << "Cleaning up payload."
                       << iclog::endl;

                if (m_indexData) {
                    PDF_AnchorList list = {m_indexData, m_indexDataCount};
                    PDF_FreeAnchors(list);
                    m_indexData = nullptr;
                }

                std::ifstream stat_stream("/proc/self/statm", std::ios_base::in);
                unsigned long size, resident, share, text, lib, data, dt;
                stat_stream >> size >> resident >> share >> text >> lib >> data >> dt;

                // Page size is usually 4KB
                long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
                wkJlog << iclog::loglevel::debug << iclog::category::LIB << iclog_FUNCTION
                       << "Cleaning up class object complete.\n"
                       << "MEM: Resident Set Size: " << (resident * page_size_kb) << " KB"
                       << iclog::endl;
            }

            void  read_file_to_blob();
            char *read_file(const char *fullPath);
            void  make_pdf_int();
            void  make_pdf_ext();

            static int cb_worker(void *p);
    };

    /**
     * @brief print_finished
     * @param print_operation
     * @param user_data
     *
     * Print finshed callback.
     */
    static void print_finished(WebKitPrintOperation *op __attribute__((unused)), gpointer user_data) {
        // Recover the implementation pointer directly
        PDFprinter_impl *impl = static_cast<PDFprinter_impl *>(user_data);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Print operation finished." << iclog::endl;

        // This is what breaks the loop in cb_worker
        if (impl->m_innerLoop) {
            g_main_loop_quit(impl->m_innerLoop);
        }

        impl->m_processing = false;
    }

    /**
     * @brief javascript_callback
     * @param web_view
     * @param result
     * @param user_data
     *
     * Grab the physical coordinates of of the
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
            wkJlog << iclog::loglevel::error << iclog::category::CORE
                   << "JavaScript error: " << error->message << iclog::endl;
            g_error_free(error);
            return;
        }

        // Convert JSCValue to string
        gchar *json_string = jsc_value_to_string(js_result);

        if (!json_string) {
            wkJlog << iclog::loglevel::error << iclog::category::CORE
                   << "Failed to convert JavaScript result to string" << iclog::endl;
            g_object_unref(js_result);
            return;
        }

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Extracted JSON: " << json_string << iclog::endl;

        // Parse JSON with json-c
        json_object *root = json_tokener_parse(json_string);
        if (!root) {
            wkJlog << iclog::loglevel::error << iclog::category::CORE
                   << "Failed to parse JSON" << iclog::endl;
            g_free(json_string);
            g_object_unref(js_result);
            return;
        }

        PDFprinter_impl *impl = static_cast<PDFprinter_impl *>(user_data);

        json_object *toc_obj, *page_obj;
        if (json_object_object_get_ex(root, "toc", &toc_obj)) {
            if (json_object_object_get_ex(toc_obj, "page", &page_obj)) {
                (impl->m_tocPage) = json_object_get_int(page_obj);
            }
        }

        json_object *indexPositions = json_object_object_get(root, "indexPositions");

        if (indexPositions && json_object_get_type(indexPositions) == json_type_array) {
            size_t len = json_object_array_length(indexPositions);

            // 1. CLEAR STALE DATA COMPLETELY
            if (impl->m_indexData) {
                PDF_AnchorList oldList = {impl->m_indexData, impl->m_indexDataCount};
                PDF_FreeAnchors(oldList);
                impl->m_indexData      = nullptr;
                impl->m_indexDataCount = 0;
            }

            // 2. ALLOCATE FRESH
            impl->m_indexDataCount = len;
            // calloc ensures all pointers (linkName, title) start as NULL
            impl->m_indexData      = (PDF_Anchor *)calloc(len, sizeof(PDF_Anchor));

            for (size_t i = 0; i < len; ++i) {
                json_object *val = json_object_array_get_idx(indexPositions, i);
                PDF_Anchor  &a   = impl->m_indexData[i];

                const char *id_str = json_object_get_string(json_object_object_get(val, "id"));
                a.linkName         = id_str ? strdup(id_str) : strdup("");

                // Map the 'index' (source) data
                a.index.xPos        = json_object_get_double(json_object_object_get(val, "x"));
                a.index.yPos        = json_object_get_double(json_object_object_get(val, "y"));
                a.index.w           = json_object_get_double(json_object_object_get(val, "width"));
                a.index.h           = json_object_get_double(json_object_object_get(val, "height"));
                a.index.page_width  = json_object_get_double(json_object_object_get(val, "page_width"));
                a.index.page_height = json_object_get_double(json_object_object_get(val, "page_height"));
                a.index.pageNo      = json_object_get_int(json_object_object_get(val, "page"));

                wkJlog << iclog::loglevel::debug << iclog::category::CORE
                       << "Finding Index: " << (id_str ? id_str : "null") << " -> page: " << a.index.pageNo
                       << " pos: (" << a.index.xPos << "," << a.index.yPos << ")" << iclog::endl;
            }
        }

        // 3. MATCH TARGETS (Using Hash Lookup)
        // 3. MATCH TARGETS (Using Hash Lookup)
        json_object *targets = json_object_object_get(root, "targetData");
        if (targets) {
            for (size_t i = 0; i < impl->m_indexDataCount; ++i) {
                PDF_Anchor  &s          = impl->m_indexData[i];
                json_object *target_val = nullptr;

                // json-c direct lookup by key (s.linkName) is much faster than a loop
                if (json_object_object_get_ex(targets, s.linkName, &target_val)) {
                    const char *t_title = json_object_get_string(json_object_object_get(target_val, "title"));

                    s.target.title       = t_title ? strdup(t_title) : strdup("");
                    s.target.xPos        = json_object_get_double(json_object_object_get(target_val, "x"));
                    s.target.yPos        = json_object_get_double(json_object_object_get(target_val, "y"));
                    s.target.w           = json_object_get_double(json_object_object_get(target_val, "width"));
                    s.target.h           = json_object_get_double(json_object_object_get(target_val, "height"));
                    s.target.page_width  = json_object_get_double(json_object_object_get(target_val, "page_width"));
                    s.target.page_height = json_object_get_double(json_object_object_get(target_val, "page_height"));
                    s.target.pageNo      = json_object_get_int(json_object_object_get(target_val, "page"));
                }
            }
        }

        for (size_t i = 0; i != impl->m_indexDataCount; ++i) {
            PDF_Anchor &a = impl->m_indexData[i];
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "\nIndex: " << a.linkName << " -> page: " << a.index.pageNo << " pos: (" << a.index.xPos << "," << a.index.yPos << ") size: (" << a.index.w << "," << a.index.h << ")\n"
                   << "Target: " << a.linkName << " -> title: " << a.target.title << " -> page: " << a.target.pageNo << " pos: (" << a.target.xPos << "," << a.target.yPos << ") size: (" << a.target.w << "," << a.target.h << ")"
                   << iclog::endl;
        }

        // Cleanup
        json_object_put(root);
        g_free(json_string);
        g_object_unref(js_result);

        // After extraction, trigger print
        webkit_print_operation_print(impl->m_print_operation);
    }

    /**
     * @brief web_view_load_changed
     * @param web_view
     * @param load_event
     * @param user_data
     *
     * Web load monitor callback
     */
    static void web_view_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, void *user_data) {

        PDFprinter_impl *impl = static_cast<PDFprinter_impl *>(user_data);

        switch (load_event) {
            case WEBKIT_LOAD_STARTED:
                wkJlog << iclog::loglevel::debug << iclog::category::CORE
                       << "WEBKIT LOAD STARTED." << iclog::endl;

                break;
            case WEBKIT_LOAD_REDIRECTED:
                break;
            case WEBKIT_LOAD_COMMITTED:
                wkJlog << iclog::loglevel::debug << iclog::category::CORE
                       << "The load is being performed. Current URI is the final one and it "
                          "won't change unless a new "
                       << "load is requested or a navigation within the same page is "
                          "performed."
                       << iclog::endl;
                break;

            case WEBKIT_LOAD_FINISHED:
                wkJlog << iclog::loglevel::debug << iclog::category::CORE
                       << "WEBKIT LOAD FINISHED - extracting positions" << iclog::endl;

                // Check if we need to extract positions (i.e., index generation)
                if (impl->m_doIndex != index_mode::OFF) {
                    // Enable JavaScript for extraction
                    WebKitSettings *view_settings = webkit_web_view_get_settings(web_view);
                    webkit_settings_set_enable_javascript(view_settings, true);

                    const char *js_to_run = (impl->m_doIndex == index_mode::ENHANCED)
                                                ? js_code_enhanced
                                                : js_code_classic;

                    // Evaluate JS to extract positions

                    wkJlog << iclog::loglevel::debug << iclog::category::CORE
                           << "Extracting coordinates using:\n"
                           << js_to_run
                           << iclog::endl;
                    webkit_web_view_evaluate_javascript(
                        web_view,
                        js_to_run, // script
                        -1,        // length (use -1 for null-terminated string)
                        NULL,      // world_name
                        NULL,      // source_uri
                        NULL,      // cancellable
                        (GAsyncReadyCallback)javascript_callback,
                        user_data
                    );

                } else {
                    // No extraction needed — proceed directly to print
                    wkJlog << iclog::loglevel::debug << iclog::category::CORE
                           << "No index extraction required — printing directly" << iclog::endl;

                    webkit_print_operation_print(impl->m_print_operation);
                }
                break;

            default:
                // Ignore other load states (STARTED, COMMITTED, etc.)
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief PDFprinter_impl::cb_worker
     * @param p - cast to pimpl
     * @return
     *
     * Callback to generate PDF from HTML
     */
    int PDFprinter_impl::cb_worker(void *p) {

        PDFprinter_impl *impl = reinterpret_cast<PDFprinter_impl *>(p);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Applying print settings" << iclog::endl;
        impl->m_print_settings = gtk_print_settings_new();
        gtk_print_settings_set_printer(impl->m_print_settings, "Print to File");
        gtk_print_settings_set(impl->m_print_settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");

        GtkPageSetup *page_setup = gtk_page_setup_new();
        gtk_page_setup_set_top_margin(page_setup, 0, GTK_UNIT_MM);
        gtk_page_setup_set_bottom_margin(page_setup, 0, GTK_UNIT_MM);

        if (impl->key_file_data != NULL) {

            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Applying page setup:\n"
                   << impl->key_file_data << iclog::endl;
            GKeyFile *key_file = g_key_file_new();
            g_key_file_load_from_data(key_file, impl->key_file_data, (gsize)-1, G_KEY_FILE_NONE, NULL);
            gtk_page_setup_load_key_file(page_setup, key_file, NULL, NULL);
            gtk_print_settings_load_key_file(impl->m_print_settings, key_file, NULL, NULL);

            g_key_file_free(key_file);
        }

        gtk_print_settings_set(impl->m_print_settings, GTK_PRINT_SETTINGS_OUTPUT_URI, impl->out_uri);

#ifdef USE_WEBKIT_6
        // 1. Create the Headless settings first
        WebKitSettings *settings = webkit_settings_new();
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

        // 2. Create the ephemeral session
        WebKitNetworkSession *session = webkit_network_session_new_ephemeral();

        // 3. Create the WebView with BOTH the Session AND the Settings in one go
        // This is the only way to ensure the child process starts "quietly"
        impl->m_web_view = WEBKIT_WEB_VIEW(
            g_object_new(
                WEBKIT_TYPE_WEB_VIEW,
                "network-session",
                session,
                "settings",
                settings,
                NULL
            )
        );

        // Cleanup local refs (the web_view now owns them)
        g_object_unref(session);
        g_object_unref(settings);
#else
        // WEBKIT 4.1 (GTK3) WAY:
        WebKitWebContext *web_context = webkit_web_context_new_ephemeral();
        impl->m_web_view              = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(web_context));
#endif

        WebKitSettings *view_settings = webkit_web_view_get_settings(impl->m_web_view);
        webkit_settings_set_enable_javascript(view_settings, false);
        webkit_settings_set_enable_page_cache(view_settings, false);
        webkit_settings_set_enable_html5_database(view_settings, false);
        webkit_settings_set_enable_html5_local_storage(view_settings, false);

        if (impl->default_stylesheet) {

            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Injecting style sheet:\n"
                   << impl->default_stylesheet << iclog::endl;
            WebKitUserContentManager *user_content_manager = webkit_user_content_manager_new();
            WebKitUserStyleSheet     *user_stylesheet      = webkit_user_style_sheet_new(
                impl->default_stylesheet,
                WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                WEBKIT_USER_STYLE_LEVEL_USER,
                NULL,
                NULL
            );
            webkit_user_content_manager_add_style_sheet(user_content_manager, user_stylesheet);
            if (user_stylesheet) {
                webkit_user_style_sheet_unref(user_stylesheet);
            }
            g_object_set_property(G_OBJECT(impl->m_web_view), "user-content-manager", (GValue *)(user_content_manager));
            g_object_unref(user_content_manager);
        } else {
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Injecting stylesheet: No explicit style sheet set; skipping:"
                   << iclog::endl;
        }

        // g_object_ref_sink(G_OBJECT(impl->m_web_view));

        // WebKit2GTK print is async - schedules work and returns immediately
        impl->m_print_operation = webkit_print_operation_new(impl->m_web_view);
        g_object_ref_sink(impl->m_print_operation);

        webkit_print_operation_set_print_settings(impl->m_print_operation, impl->m_print_settings);
        webkit_print_operation_set_page_setup(impl->m_print_operation, page_setup);
        g_signal_connect(impl->m_print_operation, "finished", G_CALLBACK(print_finished), impl);

        // INNER LOOP: Created here, runs until WebKit2GTK print completes
        impl->m_innerLoop = g_main_loop_new(nullptr, false);
        // impl->main_loop      = main_loop;
        g_signal_connect(impl->m_web_view, "load-changed", G_CALLBACK(web_view_load_changed), impl);
        if (impl->html_txt != NULL) {
            // webkit_web_view_load_html(web_view, impl->html_txt, "file:///tmp");
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Setting base URI: " << impl->in_uri
                   << iclog::endl;
            webkit_web_view_load_html(impl->m_web_view, impl->html_txt, impl->in_uri);
            // webkit_web_view_load_html(web_view, impl->html_txt, impl->base_uri);
        } else {
            webkit_web_view_load_uri(impl->m_web_view, impl->in_uri);
        }

        // Run inner loop until print callback signals completion
        g_main_loop_run(impl->m_innerLoop);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Performing PDF generation loop cleanup operations."
               << iclog::endl;

        g_signal_handlers_disconnect_by_data(impl->m_print_operation, impl);
        g_signal_handlers_disconnect_by_data(impl->m_web_view, impl);
        // --- THE MAGIC CLEANUP BLOCK ---
        if (impl->m_web_view) {
            webkit_web_view_terminate_web_process(impl->m_web_view); // Optional: force kill sub-procs
            g_object_run_dispose(G_OBJECT(impl->m_web_view));        // Properly dispose
            g_object_unref(impl->m_web_view);                        // Release memory
            impl->m_web_view = nullptr;
        }

        // 1. Kill the loop first so nothing else can trigger
        if (impl->m_innerLoop) {
            g_main_loop_unref(impl->m_innerLoop);
            impl->m_innerLoop = nullptr;
        }

        // 2. Kill the Print Operation
        if (impl->m_print_operation) {
            g_object_unref(impl->m_print_operation);
            impl->m_print_operation = nullptr;
        }

#ifndef USE_WEBKIT_6
        if (web_context) {
            g_object_unref(web_context);
            web_context = nullptr;
        }
#endif
        wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
               << "Cleanup complete; exiting worker callback."
               << iclog::endl;

        std::mutex              *wait_mutex = impl->wait_mutex;
        std::condition_variable *wait_cond  = impl->wait_cond;
        int                     *wait_data  = impl->wait_data;

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

    static void cstring_cpy(const char *src, char *&dest) {
        if (dest != nullptr) {
            delete[] dest;
            dest = nullptr;
        }

        if (!src)
            return;

        size_t len = std::strlen(src);
        dest       = new char[len + 1];
        std::memcpy(dest, src, len + 1);
    }

    static std::string generate_uuid_string() {
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
        wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
               << "UUID generated: " << uuid << iclog::endl;

        return uuid;
    }

    /**
     * @brief PDFprinter::PDFprinter
     *
     * Set default value to "file:///"
     *
     * This satisfies the "Missing Symbol" _ZN10PDFprinterC1Ev on some machines
     *
     */
    PDFprinter::PDFprinter()
        : PDFprinter::PDFprinter("file:///") {}

    /**
     * @brief PDFprinter::PDFprinter
     */
    PDFprinter::PDFprinter(const char *baseURI)
        : m_pimpl(new PDFprinter_impl()) {

        const char *actualURI = (baseURI && *baseURI) ? baseURI : "file:///";
        cstring_cpy(actualURI, m_pimpl->in_uri);
    }

    /**
     * @brief PDFprinter::~PDFprinter
     */
    PDFprinter::~PDFprinter() {

        delete m_pimpl;
    }

    /**
     * @brief PDFprinter::read_file
     * @param fullPath
     * @return The contents of the file
     *
     * This is just a utility funciton that takes in a path to a file and
     * returns its contents.
     */
    char *PDFprinter_impl::read_file(const char *fullPath) {
        // 1. Safety check for the pointer (Crucial!)
        if (fullPath == nullptr || *fullPath == '\0') {
            return nullptr;
        }

        // 2. ifstream handles const char* perfectly
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file) {
            // Logging logic (safe to use fullPath here)
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Cannot find the file specified: " << fullPath << iclog::endl;
            return nullptr;
        }

        std::streamsize size = file.tellg();
        if (size <= 0)
            return nullptr; // Handle empty files

        file.seekg(0, std::ios::beg);

        // 3. Allocate
        char *buffer = new (std::nothrow) char[size + 1];
        if (!buffer)
            return nullptr;

        // 4. Read and Terminate
        if (file.read(buffer, size)) {
            buffer[size] = '\0';
            return buffer;
        }

        // If read fails, don't leave a dangling pointer
        delete[] buffer;
        return nullptr;
    }

    /**
     * @brief PDFprinter::read_file_to_blob
     *
     * If the caller wishes to conduct post processing then we return a
     * blob rather than a file.
     *
     * This method creates the Blob and then deletes the file.
     *
     */
    void PDFprinter_impl::read_file_to_blob() {
        // 1. Safety check for out_uri
        if (!out_uri || std::strlen(out_uri) < 7) {
            wkJlog << iclog::loglevel::error << iclog::category::CORE
                   << "Invalid out_uri for blob generation" << iclog::endl;
            return;
        }

        // 2. Strip "file://" without std::string overhead
        // out_uri + 7 points to the start of the path after "file://"
        const char *path = out_uri + 7;

        // 3. Open in binary mode
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            wkJlog << iclog::loglevel::error << iclog::category::CORE
                   << "Failed to open PDF file: " << path << iclog::endl;
            return;
        }

        // 4. Size and Read
        std::streamsize size = file.tellg();
        if (size <= 0)
            return;

        file.seekg(0, std::ios::beg);

        // m_binPDF is a private std::vector<unsigned char>, so this is safe internally
        m_binPDF.resize(static_cast<size_t>(size));

        if (file.read(reinterpret_cast<char *>(m_binPDF.data()), size)) {
            wkJlog << iclog::loglevel::info << iclog::category::CORE
                   << "Generated BLOB: " << path << " size=" << m_binPDF.size() << iclog::endl;
        } else {
            wkJlog << iclog::loglevel::error << iclog::category::CORE
                   << "Error reading PDF file contents" << iclog::endl;
            m_binPDF.clear();
        }
    }

    /******************************************************************************/
    /*  PARAMETERS METHODS                                                        */
    /******************************************************************************/
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
    void PDFprinter::set_param(const char *html, const char *printSettings, const char *outFile, index_mode createIndex) {
        m_pimpl->m_doIndex = createIndex;
        cstring_cpy(html, m_pimpl->html_txt);
        cstring_cpy(outFile, m_pimpl->m_destFile);
        cstring_cpy(printSettings, m_pimpl->key_file_data);
    }

    void PDFprinter::set_param_from_file(const char *htmlFile, const char *printSettings, const char *outFile, index_mode createIndex) {
        m_pimpl->m_doIndex = createIndex;
        cstring_cpy(outFile, m_pimpl->m_destFile);
        cstring_cpy(printSettings, m_pimpl->key_file_data);

        delete[] m_pimpl->html_txt;
        m_pimpl->html_txt = m_pimpl->read_file(htmlFile ? htmlFile : "");
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
    void PDFprinter::set_param(const char *html, const char *outFile, index_mode createIndex) {
        m_pimpl->m_doIndex = createIndex;
        cstring_cpy(outFile, m_pimpl->m_destFile);
        cstring_cpy(html, m_pimpl->html_txt);
    }

    void PDFprinter::set_param_from_file(const char *htmlFile, const char *outFile, index_mode createIndex) {
        m_pimpl->m_doIndex = createIndex;
        cstring_cpy(outFile, m_pimpl->m_destFile);

        delete[] m_pimpl->html_txt;
        m_pimpl->html_txt = m_pimpl->read_file(htmlFile ? htmlFile : "");
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
    void PDFprinter::set_param(const char *html, index_mode createIndex) {
        m_pimpl->m_doIndex = createIndex;
        cstring_cpy(html, m_pimpl->html_txt);
        m_pimpl->m_makeBlob = true;
    }

    void PDFprinter::set_param_from_file(const char *htmlFile, index_mode createIndex) {
        m_pimpl->m_doIndex = createIndex;

        delete[] m_pimpl->html_txt;
        m_pimpl->html_txt = m_pimpl->read_file(htmlFile ? htmlFile : "");
    }

    /******************************************************************************/
    /*  LAYOUT                                                                    */
    /******************************************************************************/
    void PDFprinter::layout(const char *pageSize, const char *orientation) {

        // If user passed NULL, we use defaults to prevent a crash
        std::string pSize  = (pageSize != nullptr) ? pageSize : "A4";
        std::string orient = (orientation != nullptr) ? orientation : "portrait";

        PaperSize sz{"A4", 210, 297};

        // 3. Search the C-Array (using the sentinel nullptr we added)
        for (int i = 0; isoPaperSizes[i].sizeName != nullptr; ++i) {
            // C++ string knows how to compare itself to a C-string (isoPaperSizes[i].sizeName)
            if (pSize == isoPaperSizes[i].sizeName) {
                sz = isoPaperSizes[i];
                break;
            }
        }

        // Make lower case
        std::transform(orient.begin(), orient.end(), orient.begin(), [](unsigned char c) { return std::tolower(c); });
        std::string o = "portrait";
        if (orient.compare("landscape") == 0)
            o = orient;

        std::string printSettings(
            /* clang-format off */
            "[Print Settings]\n"
            "quality=high\n"
            "resolution=300\n"
            "output-file-format=pdf\n"
            "printer=Print to File\n"
            "page-set=all\n"
            "[Page Setup]\n"
            "PPDName=" + std::string(sz.sizeName) + "\n"
            "DisplayName=" + std::string(sz.sizeName) + "\n"
            "Width=" + std::to_string(sz.shortMM) + "\n"
            "Height=" + std::to_string(sz.longMM) + "\n"
            "MarginTop=0\n"
            "MarginBottom=0\n"
            "MarginLeft=0\n"
            "MarginRight=0\n"
            "Orientation=" + o + "\n"
            /* clang-format on */
        );

        cstring_cpy(printSettings.c_str(), m_pimpl->key_file_data);
    }

    void PDFprinter::layout(double width_mm, double height_mm) {
        std::string o = (width_mm > height_mm) ? "landscape" : "portrait";

        std::string szName = std::to_string(width_mm) + "x" + std::to_string(height_mm) + "mm";

        double corrected_h = std::floor(height_mm * (72.0 / 25.4)) / (72.0 / 25.4);
        double corrected_w = std::floor(width_mm * (72.0 / 25.4)) / (72.0 / 25.4);

        // We use a generic PPDName for custom sizes;
        // Cairo/WebKit just needs the raw dimensions.
        std::string printSettings(
            /* clang-format off */
            "[Print Settings]\n"
            "quality=high\n"
            "resolution=300\n"
            "output-file-format=pdf\n"
            "printer=Print to File\n"
            "page-set=all\n"
            "[Page Setup]\n"
            "PPDName=Custom\n"
            "DisplayName=" + std::string(szName) + "\n"
            "Width=" + std::to_string(corrected_w) + "\n"
            "Height=" + std::to_string(corrected_h) + "\n"
            "MarginTop=0\n"
            "MarginBottom=0\n"
            "MarginLeft=0\n"
            "MarginRight=0\n"
            "Orientation=" + o + "\n"
            /* clang-format on */
        );

        cstring_cpy(printSettings.c_str(), m_pimpl->key_file_data);
    }

    /******************************************************************************/
    /*  MAKE PDF                                                                  */
    /******************************************************************************/
    /**
     * @brief PDFprinter::make_pdf
     *
     * Generate the pdf.
     */

    void PDFprinter_impl::make_pdf_int() {

        // DIRECTLY CREATE THE PDF
        if (m_doIndex == index_mode::OFF && m_destFile) {
            // Use std::string as a local "calculator" only
            std::string full_uri  = "file://";
            full_uri             += m_destFile;
            cstring_cpy(full_uri.c_str(), out_uri);
        }
        std::string tempFile;
        // PREVENT TEMP FILE GENERATION OVERRIDE IN TEST MODE

        tempFile = "/tmp/" + generate_uuid_string();

        // POST PROCESS (index or create blob)
        if ((m_doIndex != index_mode::OFF) || m_makeBlob) {
            std::string fullUri = "file://" + tempFile;
            cstring_cpy(fullUri.c_str(), out_uri);
        }

        // MAKE THE PDF
        std::thread t([this]() {
            std::mutex              wait_mutex;
            std::condition_variable wait_cond;
            int                     wait_data = 0;
            PDFprinter_impl::wait_cond        = &wait_cond;
            PDFprinter_impl::wait_mutex       = &wait_mutex;
            PDFprinter_impl::wait_data        = &wait_data;

            // g_idle_add((GSourceFunc)cb_worker, m_pimpl);
            // Instead of g_idle_add, use a priority-aware invocation
            wkJlog << iclog::loglevel::debug << "Queueing PDF Generation Job" << iclog::endl;
            g_main_context_invoke_full(
                NULL,                   // Use default context
                G_PRIORITY_HIGH,        // JUMP TO FRONT OF QUEUE
                (GSourceFunc)cb_worker, // Your worker function
                this,                   // Data
                NULL                    // No destroy notify
            );

            {
                std::unique_lock<std::mutex> lock(wait_mutex);
                wait_cond.wait(lock, [&wait_data] { return wait_data != 0; });
            }
        });

        t.join();

        wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
               << "Exited PDF genertation process thread." << iclog::endl;

        // CREATE INDEX (if requested)
        if (!tempFile.empty()) {
            if ((m_doIndex == index_mode::CLASSIC) || (m_doIndex == index_mode::ENHANCED)) {

                index_pdf idx(m_indexData, m_indexDataCount, m_tocPage);
                idx.create_anchors(tempFile.c_str(), m_destFile);
                std::remove(tempFile.c_str());
            }
        }

        // GENERATE BLOB (if requested)
        if (m_makeBlob) {
            wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
                   << "Making BLOB" << iclog::endl;
            read_file_to_blob();
        }
    }

    void PDFprinter_impl::make_pdf_ext() {

        // DIRECTLY CREATE THE PDF
        if (m_doIndex == index_mode::OFF && m_destFile) {
            // Use std::string as a local "calculator" only
            std::string full_uri  = "file://";
            full_uri             += m_destFile;
            cstring_cpy(full_uri.c_str(), out_uri);
        }

        std::string tempFile = "/tmp/" + generate_uuid_string();

        // POST PROCESS (index or create blob)
        if ((m_doIndex != index_mode::OFF) || m_makeBlob) {
            std::string fullUri = "file://" + tempFile;
            cstring_cpy(fullUri.c_str(), out_uri);
        }

        // MAKE THE PDF
        m_processing = true;

        // Direct call (already on the correct thread)
        cb_worker(this);

        // The "ncurses-style" pump to keep the window alive
        while (m_processing) {
            g_main_context_iteration(NULL, TRUE);
        }

        // CREATE INDEX (if requested)
        if ((m_doIndex == index_mode::CLASSIC) || (m_doIndex == index_mode::ENHANCED)) {

            index_pdf idx(m_indexData, m_indexDataCount, m_tocPage);
            idx.create_anchors(tempFile.c_str(), m_destFile);
            std::remove(tempFile.c_str());
        }

        // GENERATE BLOB (if requested)
        if (m_makeBlob) {
            wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
                   << "Making BLOB" << iclog::endl;
            read_file_to_blob();
        }
    }

    void PDFprinter::make_pdf() {

        if (WKGTK_run_mode == WKGTKRunMode::UNSET) {
            // USE THE CALLERS OWN INSTANCE OF THE PRIAMRY GTK CONTEXT (No Thread)
            return m_pimpl->make_pdf_ext();
        } else {
            // USE INTERNAL INSTACNE: threaded version
            return m_pimpl->make_pdf_int();
        }
    }

    PDF_Blob PDFprinter::get_blob() {
        PDF_Blob blob = {nullptr, 0};

        if (!m_pimpl->m_binPDF.empty()) {
            blob.size = m_pimpl->m_binPDF.size();

            // Allocate raw memory for the ABI-safe return
            blob.data = (unsigned char *)malloc(blob.size);

            if (blob.data) {
                std::memcpy(blob.data, m_pimpl->m_binPDF.data(), blob.size);
            } else {
                blob.size = 0; // Ensure size is 0 if malloc failed
            }

            // Optional: Clear the internal vector if you want to ensure
            // the data only exists in one place now.
            m_pimpl->m_binPDF.clear();
            m_pimpl->m_binPDF.shrink_to_fit();
        }

        return blob;
    }

    PDF_AnchorList PDFprinter::get_anchors() {
        PDF_AnchorList list;
        list.anchors = m_pimpl->m_indexData;
        list.count   = m_pimpl->m_indexDataCount;

        // Transfer Ownership: The impl no longer cleans this up in its destructor
        m_pimpl->m_indexData         = nullptr;
        m_pimpl->m_indexDataCount    = 0;
        m_pimpl->m_indexDataCapacity = 0;

        return list;
    }
} // namespace phtml

void PDF_FreeAnchors(PDF_AnchorList list) {

    wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
           << "Cleaning up anchor indexing data"
           << iclog::endl;
    if (!list.anchors)
        return;

    for (size_t i = 0; i < list.count; ++i) {
        // 1. Free the strings (likely from strdup or malloc)
        if (list.anchors[i].linkName)
            free((void *)list.anchors[i].linkName);
        if (list.anchors[i].index.title)
            free((void *)list.anchors[i].index.title);
        if (list.anchors[i].target.title)
            free((void *)list.anchors[i].target.title);
    }
    free(list.anchors);
}

void PDF_FreeBlob(PDF_Blob blob) {
    if (blob.data) {
        free(blob.data);
    }
}
