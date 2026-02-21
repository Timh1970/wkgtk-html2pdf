#ifndef INDEX_PDF_H
#define INDEX_PDF_H
#include "ichtmltopdf++.h"

#include <podofo/podofo.h>

#ifndef PDF_API
#define PDF_API __attribute__((visibility("default")))
#endif

// #define PODOFO_010 // !!!MAKE SURE THIS IS REMOVED!!!

class PDF_API index_pdf {
    public:
        static const int UNSET = -1;

    private:
        // struct OutlineData {
        //         std::string                             title;
        //         std::shared_ptr<PoDoFo::PdfDestination> dest;
        //         std::vector<int>                        levels;
        // };
        struct OutlineData {
                std::string      title;
                std::vector<int> levels; /**< e.g., [1, 1, 1] for "1.1.1" */
#ifdef PODOFO_010
                std::shared_ptr<PoDoFo::PdfDestination> dest;
#else
                PoDoFo::PdfDestination *dest; // Or the object, depending on your 0.9 logic
#endif
        };

        const int                        m_tocPage;
        const bool                       m_debug;
        std::vector<PDFprinter::anchor> &m_links;
        std::vector<OutlineData>         outlineData;

        std::vector<int> parseNumbering(const std::string &title);
        void             do_annotation(PoDoFo::PdfMemDocument &pdfDoc);
#ifdef PODOFO_010
        void buildNestedOutlines(PoDoFo::PdfOutlines &outlines, std::vector<OutlineData> &outlineData, std::shared_ptr<PoDoFo::PdfDestination> toc);
#else
        void buildNestedOutlines(PoDoFo::PdfOutlines *pOutlines, std::vector<OutlineData> &outlineData, PoDoFo::PdfDestination *pTocDest);
#endif

    public:
        index_pdf(std::vector<PDFprinter::anchor> &links, const int tocPage, bool debug = false);

        void create_anchors(std::string sourcePath, std::string destPath);
};

#endif // INDEX_PDF_H
