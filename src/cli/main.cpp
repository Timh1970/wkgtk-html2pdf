#include <curses.h>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <systemd/sd-journal.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>

using std::string;
std::string appname;

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
    printf("*        --index                      create anchor points (experimental) *\n");
    printf("*                                                                         *\n");
    printf("***************************************************************************\n");
}

/**
 * @brief main
 * @return
 *
 * This is a test of the inplicare print library version.
 */
int main(int argc, char *argv[]) {

    appname   = argv[0];
    // SET UP LOGGING (all logging to journal)
    LOG_LEVEL = LOG_INFO;

    // HANDLE COMMAND LINE ARGUMENTS
    string infile;
    string outfile;
    string orientation;
    string pageSize;
    bool   index = false;

    typedef enum {
        DO_INDEX = 256

    } longopt;
    static struct option long_options[] = {
        {"help",        no_argument,       0, 'h'              },
        {"infile",      required_argument, 0, 'i'              },
        {"outfile",     required_argument, 0, 'o'              },
        {"orientation", required_argument, 0, 'O'              },
        {"size",        required_argument, 0, 's'              },
        {"verbose",     required_argument, 0, 'v'              },
        {"index",       no_argument,       0, longopt::DO_INDEX},
        {NULL,          0,                 0, 0                }
    };
    int value        = 0;
    int option_index = 0;

    // while ((opt = getopt(argc, argv, "i:O:o:s:v:")) != -1) {
    //     switch (opt)

    while ((value = getopt_long(
                argc,
                argv,
                "i:O:o:s:v:",
                long_options,
                &option_index
            ))
           != -1) {

        switch (value) {
            case 'h':
                help();
                exit(0);
                break;
            case 'i': // Input html
                infile = std::filesystem::current_path().string() + "/" + string(optarg);
                break;
            case 'o': // File to write out
                outfile = std::filesystem::current_path().string() + "/" + string(optarg);
                break;
            case 'O': /**< Orientation (invalid defaults to portrait) */
                if (optarg)
                    orientation = string(optarg);
                break;
            case 's': /**< Page Size (invlid defaults to A4) */
                if (optarg)
                    pageSize = string(optarg);
                break;
            case 'v': /**< number from 1 - 7 Log level (defaults to LOG INFO) */
                if (optarg)
                    LOG_LEVEL = atoi(optarg);
                break;
            case longopt::DO_INDEX:
                index = true;
                break;
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

    if (infile.empty() || outfile.empty()) {
        help();
        exit(0);
    }

    std::cout << "\nProcessing HTML: " << infile
              << "\nOrientation: " << orientation << "\nSize: " << pageSize
              << std::endl;

    jlog << iclog::loglevel::debug << iclog::category::CLI
         << "\nProcessing HTML: " << infile << "\nOrientation: " << orientation
         << "\nSize: " << pageSize << std::endl;

    PDFprinter pdf;
    // OPTION 1
    // pdf.set_param(
    //     PDFprinter::read_file(infile),
    //     PDFprinter::read_file("/usr/share/icprint/a4-portrait-pdf.page"), /**<
    //     Can be set if you need to use a custom page size */ outfile
    // );

    // OPTION 2
    /**
     * If you are using a default page size
     */
    pdf.set_param(PDFprinter::read_file(infile), outfile, index);

    /**
     * If the pageSize and orintation are empty or invalid
     * default A4 portrait is used.
     *
     * This method must be called if you are not pasing a
     * printer settings configuration sting using OPTION 1
     * above.
     */
    pdf.layout(pageSize, orientation);
    pdf.make_pdf();

    return (0);
}
