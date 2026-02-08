#ifndef INDEX_PDF_H
#define INDEX_PDF_H
#include "ichtmltopdf++.h"

#include <podofo/podofo.h>

#ifndef PDF_API
#define PDF_API __attribute__((visibility("default")))
#endif

class PDF_API index_pdf {
    private:
        const bool                       m_debug;
        std::vector<PDFprinter::anchor> &m_links;

        void do_annotation(PoDoFo::PdfMemDocument &pdfDoc);

    public:
        index_pdf(std::vector<PDFprinter::anchor> &links, bool debug = false);

        void create_anchors(std::string sourcePath, std::string destPath);
};

#endif // INDEX_PDF_H
