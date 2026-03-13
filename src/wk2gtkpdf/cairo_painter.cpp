#include "cairo_painter.h"

#include <cairo-pdf.h>
#include <cairo.h>
#include <gio/gio.h>
#include <vector>
#include <wpe/webkit.h>
#include <wpe/wpe.h>

struct cairo_painter_impl {

        cairo_surface_t    *m_surface = nullptr;
        cairo_t            *m_cr      = nullptr;
        double              m_scale;
        WebKitWebView      *m_webView = nullptr;
        std::vector<double> m_pageYOffsets;
        GMainLoop          *m_temp_loop = nullptr; // <--- The Handshake

        void render_all_pages();
};

cairo_painter::cairo_painter(const char *outfile, double width_mm, double height_mm)
    : m_pimpl(new cairo_painter_impl()) {

    // 1. Convert mm to Points (Industrial constant: 72/25.4)
    m_pimpl->m_scale  = 2.834645669;
    double width_pts  = width_mm * m_pimpl->m_scale;
    double height_pts = height_mm * m_pimpl->m_scale;

    // 2. Open the physical file handle
    // This is the 'Industrial' replacement for the GTK Print-to-File setting
    m_pimpl->m_surface = cairo_pdf_surface_create(outfile, width_pts, height_pts);

    // 3. Create the 'Artist's Hand' (The Context)
    m_pimpl->m_cr = cairo_create(m_pimpl->m_surface);
}

cairo_painter::~cairo_painter() {
    if (m_pimpl) {
        cairo_destroy(m_pimpl->m_cr);
        cairo_surface_finish(m_pimpl->m_surface); // THE CRITICAL LINE
        cairo_surface_destroy(m_pimpl->m_surface);
        delete m_pimpl;
    }
}

void cairo_painter::render_document(void *view, const double *page_offsets, size_t count) {

    if (page_offsets && count > 0) {
        m_pimpl->m_pageYOffsets.assign(page_offsets, page_offsets + count);
    }

    m_pimpl->m_webView = static_cast<WebKitWebView *>(view);
}

void cairo_painter_impl::render_all_pages() {
    // 1. Loop through the ACTUAL pages, not just anchors
    for (double &it : m_pageYOffsets) {

        // Use the absolute Y we gathered: (top + scrollY) * scale
        double y_offset_pts = m_pageYOffsets[it] * m_scale;

        m_temp_loop = g_main_loop_new(nullptr, FALSE);

        cairo_save(m_cr);

        // Slide the PDF "paper" up so the snapshot hits the right spot
        cairo_translate(m_cr, 0, -y_offset_pts);

        webkit_web_view_take_snapshot(
            m_webView,
            WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
            WEBKIT_SNAPSHOT_OPTIONS_NONE,
            nullptr, // Cancellable
            [](GObject *source, GAsyncResult *res, gpointer user_data) {
                auto   *impl  = static_cast<cairo_painter_impl *>(user_data);
                GError *error = nullptr;

                // 2. The FINISH function name change
                auto *surface = webkit_web_view_take_snapshot_finish(
                    WEBKIT_WEB_VIEW(source),
                    res,
                    &error
                );

                if (surface && !error) {
                    cairo_set_source_surface(impl->m_cr, surface, 0, 0);
                    cairo_paint(impl->m_cr);
                    cairo_surface_destroy(surface);
                }

                g_main_loop_quit(impl->m_temp_loop);
            },
            this
        );

        // Wait for the 'snapshot_finish' callback to fire
        g_main_loop_run(m_temp_loop);
        g_main_loop_unref(m_temp_loop);
        m_temp_loop = nullptr;

        // Eject the physical PDF page and reset for the next slice
        cairo_show_page(m_cr);
        cairo_restore(m_cr);
    }
}
