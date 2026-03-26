#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <systemd/sd-journal.h>
#include <unistd.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>
using std::string;

using namespace phtml;
std::string appname;
#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

const char *calibANSIA =
    "* { "
    "    box-sizing: border-box; "
    "    margin: 0; "
    "    padding: 0; "
    "    font-family: Arial, Helvetica, sans-serif; "
    "    font-size: 10pt; "
    "} "
    "@page { "
    "    size: 215.9000mm 279.4000mm; "
    "    margin: 0; "
    "} "
    "html, body { "
    "    width: 215.9000mm; "
    "    margin: 0; padding: 0; "
    "    background-color: transparent !important; "
    "} "
    ".page { "
    "    width: 215.9000mm; "
    "    height: 278.8412mm; "
    "    background-color: white !important; "
    "    display: grid; "
    "    grid-template-columns: 16.9mm 1fr 16.9mm; "
    "    grid-template-rows: 15.8mm 1fr 15.8mm; "
    "    break-after: page; "
    "    position: relative; "
    "    overflow: hidden; "
    "} "
    ".subpage { "
    "    grid-area: 2 / 2 / 3 / 3; "
    "    display: flex; "
    "    flex-direction: column; "
    "    position: relative; "
    "    overflow: hidden; "
    "    border: 1.5pt solid blue; "
    "} "
    "@media print { "
    "    .page { border: none; box-shadow: none; } "
    "    .subpage { border: 1pt solid green; } "
    "} "
    ".grid-line-inch { "
    "    position: absolute; "
    "    left: 0; "
    "    width: 100%; "
    "    height: 0.75pt; "
    "    background: red; "
    "} "
    ".grid-line-half { "
    "    position: absolute; "
    "    left: 0; "
    "    width: 100%; "
    "    height: 0.75pt; "
    "    background: lightgrey; "
    "} ";

const char *calibA4 =
    "* { "
    "    box-sizing: border-box; "
    "    margin: 0; "
    "    padding: 0; "
    "    font-family: Arial, Helvetica, sans-serif; "
    "    font-size: 10pt; "
    "} "

    "@page { "
    "    size: 209.9028mm 296.6861mm; "
    "    margin: 0; "
    "} "

    "html, body { "
    "    width: 209.9028mm; "
    "    margin: 0; padding: 0; "
    "    background-color: transparent !important; "
    "} "

    ".page { "
    "    width: 209.9028mm; "
    "    height: 296.0927mm; "
    "    background-color: white !important; "
    "    display: grid; "
    "    grid-template-columns: 16.9mm 1fr 16.9mm; "
    "    grid-template-rows: 15.8mm 1fr 15.8mm; "
    "    break-after: page; "
    "    position: relative; "
    "    overflow: hidden; "
    "} "

    ".subpage { "
    "    grid-area: 2 / 2 / 3 / 3; "
    "    display: flex; "
    "    flex-direction: column; "
    "    position: relative; "
    "    overflow: hidden; "
    "    border: 1.5pt solid blue; "
    "} "

    "@media print { "
    "    .page { border: none; box-shadow: none; } "
    "    .subpage { border: 1pt solid green; } "
    "} "

    ".grid-line { "
    "    position: absolute; "
    "    left: 0; "
    "    width: 100%; "
    "    height: 0.75pt; "
    "    background: red; "
    "} "

    ".grid-line-mm { "
    "    position: absolute; "
    "    left: 0; "
    "    width: 100%; "
    "    height: 0.75pt; "
    "    background: lightgrey; "
    "} "

    ".top-marker { "
    "    position: absolute; "
    "    top: 0; "
    "    left: 0; "
    "    width: 100%; "
    "    height: 3pt; "
    "    background: blue; "
    "} ";

std::string get_calibration_datestamp() {

    std::chrono::system_clock::time_point now       = std::chrono::system_clock::now();
    std::time_t                           in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm                               bt{};
    localtime_r(&in_time_t, &bt);
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void calibrate_ansi_a(html_tree *dom) {
    html_tree *head = dom->new_node("head");
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/ANSIA-portrait.css\"");

    // The "US Cage" Styles
    head->new_node("style")->set_node_content(calibANSIA, false);

    html_tree *body = dom->new_node("body");

    for (int p = 0; p != 150; ++p) {
        html_tree *page = body->new_node("div class=\"page\"")->new_node("div class=\"subpage\"");

        // 11 inches total height. Let's do 0.5" intervals.
        for (int i = 0; i != 22; ++i) {
            double inches = (i + 1) * 0.5;
            // Map directly to points for the 'Surgical' Y-coordinate
            double pt_top = inches * 72.0;

            const char *color_class = (i % 2 == 1) ? "grid-line-inch" : "grid-line-half";
            page->new_node_f("div class=\"%s\" style=\"top: %.1fpt\"", color_class, pt_top)
                ->set_node_content_f("%.1f in", inches);
        }

        page->new_node("div class=\"page-number\"")
            ->set_node_content_f("Page %d | ANSI A Calibration | %s", p + 1, get_calibration_datestamp().c_str());
    }
    process_nodes(dom);
}

void calibrate(html_tree *dom) {

    html_tree *head = dom->new_node("head");
    head->new_node("style")->set_node_content(calibA4, false);

    html_tree *body = dom->new_node("body");

    for (int p = 0; p != 150; ++p) {
        html_tree *page = body->new_node("div class=\"page\"")->new_node("div class=\"subpage\"");
        page->new_node("div class=\"top-marker\"");
        for (int i = 0; i != 30; ++i) {
            page->new_node_f("div class=\"grid-line\" style=\"top: %dmm\"", (i + 1) * 10)->set_node_content_f("%dmm", (i + 1) * 10);
            for (int mm = 0; mm != 10; ++mm) {
                page->new_node_f("div class=\"grid-line-mm\" style=\"top: %dmm\"", (i + 1) * 10 + (mm + 1));
            }
        }
        page->new_node("div class=\"page-number\"")->set_node_content_f("page %d - Calibration timestamp %s", p + 1, get_calibration_datestamp().c_str());
    }

    process_nodes(dom);
}

void help() {
    printf("\n");
    printf("Usage:   %s [options]\n", appname.c_str());
    printf("***************************************************************************\n");
    printf("*                                                                         *\n");
    printf("* Options:                                                                *\n");
    printf("*    -h  --help                       Show this message                   *\n");
    printf("*    -v  --verbose X                  Set log level (1 - 7)               *\n");
    printf("*    -i  --infile myfile.html         name of source (HTML) file          *\n");
    printf("*    -o  --outfile myfile.pdf         name of file to generate            *\n");
    printf("*    -O  --orientation                \"portrait\" or \"landscape\"           *\n");
    printf("*        --index                      create anchor points                *\n");
    printf("*                                       \"classic\" or \"enhanced\"           *\n");
    printf("*    -r  --relative-uri               look for assets in current folder   *\n");
    printf("*        --version                    show the version                    *\n");
    printf("*                                                                         *\n");
    printf("*        --calibrate                  ISOgenerate a self test pdf         *\n");
    printf("*                                     \"ISO\" or \"US\"                       *\n");
    printf("*                                                                         *\n");
    printf("***************************************************************************\n");
}

/**
 * @brief main
 * @return
 *
 * Command line interface for generation of PDF's from HTML
 *
 * @note This is a simple bridge to the API with little functionality
 * beyond the parsing of command line arguments
 */
int main(int argc, char *argv[]) {

    appname   = argv[0];
    // SET UP LOGGING (all logging to journal)
    LOG_LEVEL = LOG_INFO;

    // HANDLE COMMAND LINE ARGUMENTS
    string     infile;
    string     outfile;
    string     orientation = "portrait";
    string     pageSize    = "A4";
    index_mode idxMode     = index_mode::OFF;
    string     baseURI     = "file:///";

    typedef enum {
        DO_INDEX = 256,
        OPT_VERSION,
        OPT_CALIBRATE

    } longopt;
    static struct option long_options[] = {
        {"help",         no_argument,       0, 'h'                   },
        {"infile",       required_argument, 0, 'i'                   },
        {"outfile",      required_argument, 0, 'o'                   },
        {"orientation",  required_argument, 0, 'O'                   },
        {"relative-uri", no_argument,       0, 'r'                   },
        {"size",         required_argument, 0, 's'                   },
        {"verbose",      required_argument, 0, 'v'                   },
        {"index",        required_argument, 0, longopt::DO_INDEX     },
        {"version",      no_argument,       0, longopt::OPT_VERSION  },
        {"calibrate",    required_argument, 0, longopt::OPT_CALIBRATE},
        {NULL,           0,                 0, 0                     }
    };
    int  value        = 0;
    int  option_index = 0;
    bool doCalibrate  = false;
    // CALIBRATION
    void (*calib)(phtml::html_tree *);
    calib = &calibrate;

    while ((value = getopt_long(
                argc,
                argv,
                "i:O:o:s:v:rh",
                long_options,
                &option_index
            ))
           != -1) {

        switch (value) {
            case 'h': {
                help();
                exit(0);
                break;
            }
            case 'i': { // Input html
                infile = std::filesystem::current_path().string() + "/" + string(optarg);
                break;
            }
            case 'o': { // File to write out
                outfile = std::filesystem::current_path().string() + "/" + string(optarg);
                break;
            }
            case 'O': { /**< Orientation (invalid defaults to portrait) */
                if (optarg)
                    orientation = string(optarg);
                break;
            }
            case 'r': { /**< Relative path */
                baseURI = "file://" + std::filesystem::current_path().string() + "/";
                break;
            }
            case 's': { /**< Page Size (invlid defaults to A4) */
                if (optarg)
                    pageSize = string(optarg);
                break;
            }
            case 'v': { /**< number from 1 - 7 Log level (defaults to LOG INFO) */
                if (optarg)
                    LOG_LEVEL = atoi(optarg);
                break;
            }
            case longopt::DO_INDEX: { /**< Index defaults to off */
                string imode(optarg);
                if (imode.compare("classic") == 0)
                    idxMode = index_mode::CLASSIC;
                if (imode.compare("enhanced") == 0)
                    idxMode = index_mode::ENHANCED;
                break;
            }
            case longopt::OPT_VERSION: {

                std::cout << appname << ": v" << APP_VERSION << std::endl;
                std::cout << "Engine: " << wk2gtkpdf_version() << std::endl;
                return 0;

                exit(0);
                break;
            }
            case longopt::OPT_CALIBRATE: {

                string calibType(optarg);
                if (calibType.compare("US") == 0) {
                    pageSize = "ANSIA";
                    calib    = &calibrate_ansi_a;
                }
                std::cout << "Generating calibration PDF (150page " << pageSize << " ruled)" << std::endl;
                doCalibrate = true;
                break;
            }
            default:
                break;
        }
    }

    // REDIRECT WEBKIT LOGGING TO SYSLOG
    dup2(sd_journal_stream_fd(argv[0], LOG_LEVEL, 1), STDERR_FILENO);
    setlogmask(LOG_UPTO(LOG_LEVEL));
    setlocale(LC_CTYPE, "en_GB.UTF-8");

    // INITIALISE WEBKIT2GTK
    icGTK::init();

    if (doCalibrate) {
        PDFprinter  pdf("file:///");
        const char *calbFile = strdup((std::filesystem::current_path().string() + "/wkgtk-html2pdf-cal-" + get_calibration_datestamp() + ".pdf").c_str());
        html_tree   dom("html");
        calib(&dom);
        const char *html = dom.get_html();

        std::ofstream file(std::filesystem::current_path().string() + "/wkgtk-html2pdf-cal-" + get_calibration_datestamp() + ".html");
        if (file) {
            file << html;
            file.close();
        }
        pdf.set_param(
            html,
            calbFile
        );
        pdf.layout(pageSize.c_str(), "portrait");
        pdf.make_pdf();
        std::cout << "Calibration document generated - " << calbFile
                  << "To test measurements on a physical device enusre that scaling and fit is disabled."
                  << std::endl;
        PDF_FreeHTML(html);
        exit(0);
    }

    if (infile.empty() || outfile.empty()) {
        help();
        exit(0);
    }

    std::cout << "\nProcessing HTML: " << infile
              << "\nOrientation: " << orientation << "\nSize: " << pageSize
              << std::endl;

    wkJlog << iclog::loglevel::debug << iclog::category::CLI
           << "\nProcessing HTML: " << infile << "\nOrientation: " << orientation
           << "\nSize: " << pageSize << iclog::endl;

    PDFprinter pdf(baseURI.c_str());
    // OPTION 1 (depreciated/removed pending final testing) No longer necessary
    // pdf.set_param(
    //     PDFprinter::read_file(infile),
    //     PDFprinter::read_file("/usr/share/icprint/a4-portrait-pdf.page"), /**<
    //     Can be set if you need to use a custom page size */ outfile
    // );

    // OPTION 2
    /**
     * If you are using a default page size
     */
    // pdf.set_param(PDFprinter::read_file(infile), outfile, idxMode);
    pdf.set_param_from_file(infile.c_str(), outfile.c_str(), idxMode);

    /**
     * If the pageSize and orintation are empty or invalid
     * default A4 portrait is used.
     *
     * This method must be called if you are not pasing a
     * printer settings configuration sting using OPTION 1
     * above.
     */
    pdf.layout(pageSize.c_str(), orientation.c_str());
    pdf.make_pdf();

    return (0);
}
