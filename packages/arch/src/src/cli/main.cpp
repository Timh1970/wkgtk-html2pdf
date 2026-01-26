#include <curses.h>
#include <filesystem>
#include <iostream>
#include <systemd/sd-journal.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>

using std::string;

/**
 * @brief main
 * @return
 *
 * This is a test of the inplicare print library version.
 */
int main(int argc, char *argv[]) {

    // SET UP LOGGING (all logging to journal)
    LOG_LEVEL = LOG_INFO;

    // HANDLE COMMAND LINE ARGUMENTS
    int    opt = 0;
    string infile;
    string outfile;
    string orientation;
    string pageSize;
    while ((opt = getopt(argc, argv, "i:O:o:s:v:")) != -1) {
        switch (opt) {
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
        std::cout << "USAGE: " << argv[0]
                  << " -i infile.html -o outfile.pdf -O orientation -s page size -v (Log level 1 - 7)\n"
                  << std::endl;
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
    pdf.set_param(PDFprinter::read_file(infile), outfile);

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
