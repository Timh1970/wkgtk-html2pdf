#ifndef GTKPRINT_H
#define GTKPRINT_H
#include <condition_variable>
#include <mutex>
#include <string>
#include <systemd/sd-bus.h>
#include <thread>
#include <vector>
#include <webkit2/webkit2.h>

#ifndef PDF_API
#define PDF_API __attribute__((visibility("default")))
#endif

/**
 * @brief The XvfbMode enum
 *
 * Modes to run xvfb in.
 *
 * @note START_STOP may be used in some edge cases, but is only
 * really intened for testing.  The recommended way to run the
 * aplicaiton is to leave xvfb running.
 *
 * @note xvfb Is only required on headless servers, it will not be started
 * unless it is needed.
 *
 */
enum class XvfbMode {
    START_STOP,   // Start on demand, stop on exit
    KEEP_RUNNING, // Start on demand, keep running
};

class WKGTK_init {
    private:
        GMainLoop              *loop = nullptr;
        std::thread             glob_Thread;
        std::mutex              init_mutex;
        std::condition_variable init_cond;

    public:
        WKGTK_init();
        ~WKGTK_init();
};

/**
 * @brief The icGTK class
 * Test and set up the environment and create an instance of WKGTK_init
 * if all is good.
 *
 * @note this is a singleton class.
 */
class icGTK {

    private:
        WKGTK_init tk;
        XvfbMode   runMode;

        WKGTK_init  handle_xvfb_daemon();
        std::string check_xvfb(sd_bus *bus, const std::string &service);
        bool        start_service(sd_bus *bus);
        bool        stop_service(sd_bus *bus);

        icGTK(XvfbMode runMode);
        ~icGTK();

    public:
        PDF_API static icGTK &init(XvfbMode runMode = XvfbMode::KEEP_RUNNING);
        PDF_API               icGTK(const icGTK &)     = delete;
        PDF_API icGTK        &operator=(const icGTK &) = delete;
};

enum class index_mode {
    OFF,
    CLASSIC,
    ENHANCED
};

/**
 * @brief The html2pdf_params class
 *
 * A definition payload that is passed to the function that creates the pdf
 */
struct html2pdf_params {
        const char              *in_uri;
        const char              *html_txt;
        const char              *base_uri;
        const char              *out_uri;
        const char              *key_file_data;
        const char              *default_stylesheet;
        std::mutex              *wait_mutex;
        std::condition_variable *wait_cond;
        int                     *wait_data;
        void                    *indexData; /**< std::vector<PDFprinter::anchor> */
        index_mode               doIndex;
        int                     *tocPage;
};

/**
 * @class PDFprinter
 * @brief HTML to PDF converter using WebKit2GTK.
 *
 * Converts HTML content to PDF with support for:
 * - Custom page setup (size, margins, orientation)
 * - GTK print settings
 * - Binary blob output (for post-processing)
 *
 * @note Temporary files are used internally and cleaned up automatically.
 *
 * @example
 * @code
 * PDFprinter printer;
 * printer.set_param(html);
 * printer.layout(PDFprinter::PageSize::A4);
 * printer.generate_pdf();
 * auto pdf_data = std::move(printer).get_blob();
 * @endcode
 */
class PDF_API PDFprinter {
    public:
        struct PaperSize {
                std::string sizeName;
                uint        shortMM;
                uint        longMM;
        };

        struct linkData {
                std::string title;
                double      xPos;
                double      yPos;
                double      w;
                double      h;
                double      page_width;
                double      page_height;
                int         pageNo;
        };

        struct anchor {
                std::string linkName;
                linkData    index;
                linkData    target;
        };

        typedef std::vector<unsigned char> blob;

        /**
         * @brief isoPaperSizes
         *
         * This is a list of standard well known page
         * sizes (they are not all ISO)
         *
         * @todo remove this and maintain a hidden JSON
         * file in the /usr/share/wk2gtkpdf to retain
         * the template data.
         *
         * Parse that file instead of this struct to
         * allow users to add custom paper sizes using
         * the template_maker utility.
         *
         */
        const std::vector<PaperSize> isoPaperSizes = {
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
            {"ArchE",   914,  1219}
        };

    private:
        char      *in_uri;
        char      *html_txt;
        char      *base_uri;
        char      *out_uri;
        char      *key_file_data;
        char      *default_stylesheet;
        bool       m_makeBlob;
        index_mode m_doIndex;

        std::string m_destFile;

        /**
         * @brief payload
         * Payload only contains pointers to the values, not the
         * values themselves.
         */
        html2pdf_params payload;

        blob m_binPDF;

        /**
         * @brief m_indexData
         *
         * Coordinates for generating index and anchor points
         */
        std::vector<anchor> m_indexData;
        int                 m_tocPage;

        std::string generate_uuid_string();
        void        to_cstring(const std::string &str, char *&cstr);
        void        read_file_to_blob();

    public:
        PDFprinter(std::string baseURI = "file:///");
        ~PDFprinter();
        void set_param(std::string html, std::string printSettings, std::string outFile, index_mode createIndex = index_mode::OFF);
        void set_param(std::string html, std::string outFile, index_mode createIndex = index_mode::OFF);
        void set_param(std::string html, index_mode createIndex = index_mode::OFF);
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
        void make_pdf();
        void layout(std::string pageSize, std::string oreintation);

        static std::string read_file(const std::string &fullPath);

        /**
         * @brief get_blob
         * @return a Binary Large OBject in format
         *
         * - vector<char>
         *
         * Can be used for post processing
         *
         */
        const blob &get_blob() const;
        /**
         * @brief get_blob
         * @return - ownership of the BLOB
         */
        blob      &&get_blob();

        /**
         * @brief get_anchor_data
         * @return - ownership of the index data array.
         */
        std::vector<PDFprinter::anchor> &&get_anchor_data();
};

#endif // GTKPRINT_H
