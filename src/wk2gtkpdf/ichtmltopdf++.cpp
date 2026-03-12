#include "ichtmltopdf++.h"
#include "iclog.h"
#include "index_pdf.h"

#include <algorithm>
#include <atomic>
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
        {"A0",      841,  1189},
        {"A1",      594,  841 },
        {"A2",      420,  594 },
        {"A3",      297,  420 },
        {"A4",      210,  297 },
        {"A5",      148,  210 },
        {"A6",      105,  148 },
        {"A7",      74,   105 },
        {"A8",      52,   74  },
        {"A9",      37,   52  },
        {"A10",     26,   37  },
        {"SRA0",    900,  1280},
        {"SRA1",    640,  900 },
        {"SRA2",    450,  640 },
        {"SRA3",    320,  450 },
        {"SRA4",    225,  320 },
        {"B0",      1000, 1414},
        {"B1",      707,  1000},
        {"B2",      500,  707 },
        {"B3",      353,  500 },
        {"B4",      250,  353 },
        {"B5",      176,  250 },
        {"B6",      125,  176 },
        {"B7",      88,   125 },
        {"B8",      62,   88  },
        {"B9",      44,   62  },
        {"B10",     31,   44  },
        {"C0",      917,  1297},
        {"C1",      648,  917 },
        {"C2",      458,  648 },
        {"C3",      324,  458 },
        {"C4",      229,  324 },
        {"C5",      162,  229 },
        {"C6",      114,  162 },
        {"C7",      81,   114 },
        {"C8",      57,   81  },
        {"C9",      40,   57  },
        {"C10",     28,   40  },
        {"ANSIA",   216,  279 },
        {"ANSIB",   279,  432 },
        {"ANSIC",   432,  559 },
        {"ANSID",   559,  864 },
        {"ANSIE",   864,  1118},
        {"Letter",  216,  279 },
        {"Legal",   216,  356 },
        {"Tabloid", 279,  432 },
        {"ArchA",   229,  305 },
        {"ArchB",   305,  457 },
        {"ArchC",   457,  610 },
        {"ArchD",   610,  914 },
        {"ArchE",   914,  1219},
        {nullptr,   0,    0   }
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
            GtkPrintSettings        *m_print_settings   = nullptr;
            WebKitPrintOperation    *m_print_operation  = nullptr;
            GMainLoop               *main_loop          = nullptr;
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
            // void  add_index_entry(
            //      const char *id,
            //      double      x,
            //      double      y,
            //      double      w,
            //      double      h,
            //      double      pw,
            //      double      ph,
            //      int         pg
            //  );
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
        if (impl->main_loop) {
            g_main_loop_quit(impl->main_loop);
        }

        impl->m_processing = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /**
     * @brief INDEXABLE PDF
     */

    // --- EXPERIMENTAL CODE START ---
    // Added JavaScript extraction for index and anchor positions
    // This code is experimental and may need refinement

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

            // Clear any potential stale data
            if (impl->m_indexData) {
                PDF_AnchorList oldList = {impl->m_indexData, impl->m_indexDataCount};
                PDF_FreeAnchors(oldList);
                impl->m_indexData = nullptr;
            }

            // Reallocate raw array to match JSON count
            impl->m_indexData      = (PDF_Anchor *)realloc(impl->m_indexData, sizeof(PDF_Anchor) * len);
            impl->m_indexDataCount = len;

            for (size_t i = 0; i < len; ++i) {
                json_object *val = json_object_array_get_idx(indexPositions, i);
                PDF_Anchor  &a   = impl->m_indexData[i];
                std::memset(&a, 0, sizeof(PDF_Anchor));

                const char *id = json_object_get_string(json_object_object_get(val, "id"));
                a.linkName     = id ? strdup(id) : strdup("");

                // Map the 'index' (source) data
                a.index.xPos        = json_object_get_double(json_object_object_get(val, "x"));
                a.index.yPos        = json_object_get_double(json_object_object_get(val, "y"));
                a.index.w           = json_object_get_double(json_object_object_get(val, "width"));
                a.index.h           = json_object_get_double(json_object_object_get(val, "height"));
                a.index.page_width  = json_object_get_double(json_object_object_get(val, "page_width"));
                a.index.page_height = json_object_get_double(json_object_object_get(val, "page_height"));
                a.index.pageNo      = json_object_get_int(json_object_object_get(val, "page"));

                wkJlog << iclog::loglevel::debug << iclog::category::CORE
                       << "Finding Index: " << (id ? id : "null") << " -> page: " << a.index.pageNo
                       << " pos: (" << a.index.xPos << "," << a.index.yPos << ")" << iclog::endl;
            }
        }

        // Extract targetData (the target id)
        json_object *targets = json_object_object_get(root, "targetData");

        if (targets) {
            for (size_t i = 0; i < impl->m_indexDataCount; ++i) {
                PDF_Anchor &s = impl->m_indexData[i];

                // ...and for EACH anchor, we hunt through the JSON targets
                json_object_object_foreach(targets, key, val) {
                    if (std::strcmp(s.linkName, key) == 0) {
                        // MATCH FOUND: This anchor 's' now knows its 'target'
                        const char *title = json_object_get_string(json_object_object_get(val, "title"));

                        // Clean up any old ghost strings before strdup
                        if (s.target.title)
                            free((void *)s.target.title);

                        s.target.title       = title ? strdup(title) : strdup("");
                        s.target.xPos        = json_object_get_double(json_object_object_get(val, "x"));
                        s.target.yPos        = json_object_get_double(json_object_object_get(val, "y"));
                        s.target.w           = json_object_get_double(json_object_object_get(val, "width"));
                        s.target.h           = json_object_get_double(json_object_object_get(val, "height"));
                        s.target.page_width  = json_object_get_double(json_object_object_get(val, "page_width"));
                        s.target.page_height = json_object_get_double(json_object_object_get(val, "page_height"));
                        s.target.pageNo      = json_object_get_int(json_object_object_get(val, "page"));

                        // WE DO NOT BREAK the outer loop, because another anchor
                        // might point here too!
                        // We can break this INNER loop though, as we found the target for 's'.
                        break;
                    }
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

                    wkJlog << iclog::loglevel::debug << iclog::category::CORE
                           << "Extracting coordinates using:\n"
                           << js_to_run
                           << iclog::endl;
                    // Evaluate JS to extract positions
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
     * @brief cb_worker
     * @param p
     * @return
     *
     * This is a callback for Webkit2GTK that actually generates the PDF
     *
     * @note It is **NOT** threadsafe as Webkit2GTK is event driven and handles
     * calls in a queue.
     */
    int PDFprinter_impl::cb_worker(void *p) {

        PDFprinter_impl *impl = reinterpret_cast<PDFprinter_impl *>(p);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Applying print settings" << iclog::endl;
        GtkPrintSettings *print_settings = gtk_print_settings_new();
        gtk_print_settings_set_printer(print_settings, "Print to File");
        gtk_print_settings_set(print_settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");

        GtkPageSetup *page_setup = gtk_page_setup_new();

        if (impl->key_file_data != NULL) {

            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Applying page setup:\n"
                   << impl->key_file_data << iclog::endl;
            GKeyFile *key_file = g_key_file_new();
            g_key_file_load_from_data(key_file, impl->key_file_data, (gsize)-1, G_KEY_FILE_NONE, NULL);
            gtk_page_setup_load_key_file(page_setup, key_file, NULL, NULL);
            gtk_print_settings_load_key_file(print_settings, key_file, NULL, NULL);

            g_key_file_free(key_file);
        }

        gtk_print_settings_set(print_settings, GTK_PRINT_SETTINGS_OUTPUT_URI, impl->out_uri);

        impl->m_print_settings = print_settings;

        // WebKitWebContext         *web_context          = webkit_web_context_new_ephemeral();
        WebKitWebView *web_view = 0;

#ifdef USE_WEBKIT_6
        // 1. Create the Headless settings first
        WebKitSettings *settings = webkit_settings_new();
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

        // 2. Create the ephemeral session
        WebKitNetworkSession *session = webkit_network_session_new_ephemeral();

        // 3. Create the WebView with BOTH the Session AND the Settings in one go
        // This is the only way to ensure the child process starts "quietly"
        web_view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, "network-session", session, "settings", settings, NULL));

        // Cleanup local refs (the web_view now owns them)
        g_object_unref(session);
        g_object_unref(settings);
#else
        // WEBKIT 4.1 (GTK3) WAY:
        WebKitWebContext *web_context = webkit_web_context_new_ephemeral();
        web_view                      = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(web_context));
#endif

        WebKitUserContentManager *user_content_manager = 0;
        WebKitUserStyleSheet     *user_stylesheet      = 0;

        // web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(web_context));

        WebKitSettings *view_settings = webkit_web_view_get_settings(web_view);
        webkit_settings_set_enable_javascript(view_settings, false);
        webkit_settings_set_enable_page_cache(view_settings, false);
        webkit_settings_set_enable_html5_database(view_settings, false);
        webkit_settings_set_enable_html5_local_storage(view_settings, false);

        if (impl->default_stylesheet) {

            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Injecting style sheet:\n"
                   << impl->default_stylesheet << iclog::endl;
            user_content_manager = webkit_user_content_manager_new();
            user_stylesheet      = webkit_user_style_sheet_new(
                impl->default_stylesheet,
                WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                WEBKIT_USER_STYLE_LEVEL_USER,
                NULL,
                NULL
            );
            webkit_user_content_manager_add_style_sheet(user_content_manager, user_stylesheet);
            g_object_set_property(G_OBJECT(web_view), "user-content-manager", (GValue *)(user_content_manager));
        } else {
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Injecting stylesheet: No explicit style sheet set; skipping:"
                   << iclog::endl;
        }

        g_object_ref_sink(G_OBJECT(web_view));

        // WebKit2GTK print is async - schedules work and returns immediately
        WebKitPrintOperation *print_operation = webkit_print_operation_new(web_view);
        webkit_print_operation_set_print_settings(print_operation, print_settings);
        webkit_print_operation_set_page_setup(print_operation, page_setup);
        g_signal_connect(print_operation, "finished", G_CALLBACK(print_finished), impl);
        impl->m_print_operation = print_operation;

        // INNER LOOP: Created here, runs until WebKit2GTK print completes
        GMainLoop *main_loop = g_main_loop_new(nullptr, false);
        impl->main_loop      = main_loop;
        g_signal_connect(web_view, "load-changed", G_CALLBACK(web_view_load_changed), impl);
        if (impl->html_txt != NULL) {
            // webkit_web_view_load_html(web_view, impl->html_txt, "file:///tmp");
            wkJlog << iclog::loglevel::debug << iclog::category::CORE
                   << "Setting base URI: " << impl->in_uri
                   << iclog::endl;
            webkit_web_view_load_html(web_view, impl->html_txt, impl->in_uri);
            // webkit_web_view_load_html(web_view, impl->html_txt, impl->base_uri);
        } else {
            webkit_web_view_load_uri(web_view, impl->in_uri);
        }

        // Run inner loop until print callback signals completion
        g_main_loop_run(main_loop);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Performing PDF generation loop cleanup operations."
               << iclog::endl;

#ifdef USE_WEBKIT_6
        // GTK4: No gtk_widget_destroy. Unreffing the view triggers its
        // internal cleanup of the NetworkSession and WebsiteDataManager.
        if (WEBKIT_IS_WEB_VIEW(web_view)) {
            g_object_unref(web_view);
            web_view = nullptr;
        }
#else
        // WebKit 4.1 / GTK3: Manual destruction and context cleanup
        if (GTK_IS_WIDGET(web_view)) {
            gtk_widget_destroy(GTK_WIDGET(web_view));
            web_view = nullptr;
        }
        if (web_context) {
            g_object_unref(web_context);
            web_context = nullptr;
        }
#endif

        // Print Operation cleanup (Identical in both versions)
        if (impl->m_print_operation) {
            g_object_unref(impl->m_print_operation);
            impl->m_print_operation = nullptr;
        }

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

        wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
               << "Cleanup complete; exiting worker callback."
               << iclog::endl;

        return G_SOURCE_REMOVE;
    }

    static void cstring_cpy(const char *src, char *&dest) {
        delete[] dest;
        dest = nullptr;

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

    // void PDFprinter_impl::add_index_entry(const char *id, double x, double y, double w, double h, double pw, double ph, int pg) {
    //     // 1. Grow the array if needed (Manual Vector)
    //     if (m_indexDataCount >= m_indexDataCapacity) {
    //         m_indexDataCapacity = (m_indexDataCapacity == 0) ? 10 : m_indexDataCapacity * 2;
    //         m_indexData         = (PDF_Anchor *)realloc(m_indexData, sizeof(PDF_Anchor) * m_indexDataCapacity);
    //     }

    //     // 2. Initialise the new slot
    //     PDF_Anchor &a = m_indexData[m_indexDataCount++];
    //     std::memset(&a, 0, sizeof(PDF_Anchor));

    //     // 3. Store the data (Deep copy the ID string)
    //     a.linkName          = strdup(id ? id : "");
    //     a.index.xPos        = x;
    //     a.index.yPos        = y;
    //     a.index.w           = w;
    //     a.index.h           = h;
    //     a.index.page_width  = pw;
    //     a.index.page_height = ph;
    //     a.index.pageNo      = pg;
    // }

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

    void PDFprinter::layout(unsigned width, unsigned height) {
        std::string o      = (width > height) ? "landscape" : "portrait";
        std::string szName = std::to_string(width) + "x" + std::to_string(height) + "mm";
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
            "Width=" + std::to_string(width) + "\n"
            "Height=" + std::to_string(height) + "\n"
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

        std::string tempFile = "/tmp/" + generate_uuid_string();

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
