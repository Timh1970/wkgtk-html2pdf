#ifndef GTKPRINT_H
#define GTKPRINT_H
#include "ichtmltopdf_int.h" // IWYU pragma: keep

#include <cstddef>
#ifndef PDF_API
#define PDF_API __attribute__((visibility("default")))
#endif

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

typedef struct _GMainLoop GMainLoop;
struct sd_bus;

#ifdef __cplusplus
extern "C" {
#endif

struct PDF_LinkData {
        const char *title;
        double      xPos;
        double      yPos;
        double      w;
        double      h;
        double      page_width;
        double      page_height;
        int         pageNo;
};

struct PDF_Anchor {
        const char  *linkName;
        PDF_LinkData index;
        PDF_LinkData target;
};

struct PDF_AnchorList {
        PDF_Anchor *anchors;
        size_t      count;
};

struct PDF_Blob {
        unsigned char *data;
        size_t         size;
};

struct PaperSize {
        const char *sizeName;
        unsigned    shortMM;
        unsigned    longMM;
};

PDF_API const char *wk2gtkpdf_version();
PDF_API void        PDF_FreeAnchors(PDF_AnchorList list);
PDF_API void        PDF_FreeBlob(PDF_Blob blob);

#ifdef __cplusplus
}
#endif

enum class index_mode {
    OFF,
    CLASSIC,
    ENHANCED
};

namespace phtml {
    struct PDFprinter_impl;

    class PDF_API PDFprinter {
        public:
            PDF_API PDFprinter();
            PDF_API PDFprinter(const char *baseURI);
            PDF_API ~PDFprinter();
            PDF_API void     set_param(const char *html, const char *printSettings, const char *outFile, index_mode createIndex = index_mode::OFF);
            PDF_API void     set_param(const char *html, const char *outFile, index_mode createIndex = index_mode::OFF);
            PDF_API void     set_param(const char *html, index_mode createIndex = index_mode::OFF);
            PDF_API void     set_param_from_file(const char *htmlFile, const char *printSettings, const char *outFile, index_mode createIndex = index_mode::OFF);
            PDF_API void     set_param_from_file(const char *htmlFile, const char *outFile, index_mode createIndex = index_mode::OFF);
            PDF_API void     set_param_from_file(const char *htmlFile, index_mode createIndex = index_mode::OFF);
            /**
             * @brief PDFprinter::make_pdf
             *
             * Handle the creation of a pdf.
             *
             * Assign all the variables to a payload object and then put it in the
             * queue for webkit2gtk to handle the request.
             *
             * Await completion before exiting.
             */
            PDF_API void     make_pdf();
            PDF_API void     layout(const char *pageSize, const char *oreintation);
            PDF_API void     layout(unsigned width, unsigned height);
            /**
             * @brief get_blob
             * Returns a Binary Large Object (PDF data).
             *
             * @note OWNERSHIP: The caller takes ownership of the allocated memory.
             * @warning You MUST call PDF_FreeBlob() when finished to prevent memory leaks.
             *
             * @return A PDF_Blob struct containing the data pointer and size.
             */
            PDF_API PDF_Blob get_blob();

            /**
             * @brief get_anchors
             * Returns the list of anchors and links parsed from the HTML.
             *
             * @note OWNERSHIP: The caller takes ownership of the array and all nested strings.
             * @warning You MUST call PDF_FreeAnchors() to safely deep-clean this structure.
             *
             * @return A PDF_AnchorList struct.
             */
            PDF_API PDF_AnchorList get_anchors();

        private:
            // This is the ONLY variable the user's compiler sees.
            // It is 8 bytes. It never changes size.
            PDFprinter_impl *m_pimpl;
    };
} // namespace phtml

#endif // GTKPRINT_H
