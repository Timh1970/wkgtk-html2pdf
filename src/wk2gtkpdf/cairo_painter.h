#ifndef CAIRO_PAINTER_H
#define CAIRO_PAINTER_H
#include "ichtmltopdf++.h"

struct cairo_painter_impl;

class cairo_painter {
    public:
        cairo_painter(const char *outfile, double width_mm, double height_mm);

        ~cairo_painter();
        void render_document(void *view, const double *page_offsets, size_t count);

    private:
        cairo_painter_impl *m_pimpl;
};

#endif // CAIRO_PAINTER_H
