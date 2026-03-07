#ifndef INDEX_PDF_H
#define INDEX_PDF_H
#include "ichtmltopdf++.h"

struct index_pdf_impl;

class index_pdf {
    public:
        static const int UNSET = -1;

        // Use the ABI-safe raw pointer and count here
        index_pdf(const PDF_Anchor *links, size_t count, int tocPage, bool debug = false);
        ~index_pdf();

        // Changed to const char* for ABI safety
        void create_anchors(const char *sourcePath, const char *destPath);

    private:
        // Move ALL PoDoFo and std::vector members into a Pimpl here too
        struct index_pdf_impl *m_pimpl;
};

// #define PODOFO_010 // !!!MAKE SURE THIS IS REMOVED!!!

#endif // INDEX_PDF_H
