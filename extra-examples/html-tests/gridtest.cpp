#include <filesystem>
#include <fstream>
#include <systemd/sd-journal.h>
#include <unistd.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>

using namespace phtml;

int main(int argc, char *argv[]) {

    LOG_LEVEL = LOG_WARNING;
    // REDIRECT WEBKIT LOGGING TO SYSLOG
    dup2(sd_journal_stream_fd(argv[0], LOG_LEVEL, 1), STDERR_FILENO);
    setlogmask(LOG_UPTO(LOG_LEVEL));
    setlocale(LC_CTYPE, "en_GB.UTF-8");
    icGTK::init();

    html_tree dom("html");

    html_tree *head = dom.new_node("head");
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait.css\"");
    head->new_node("style")->set_node_content(
        "/* The Grid: 1px red line every 10mm */ "
        ".grid-line { "
        "    position: absolute; "
        "    left: 0; "
        "    width: 100%; "
        "    height: 1pt; "
        "    background: red; "
        "} "

        // "/* The Grid: 1px red line every 10mm */ "
        // ".grid-line { "
        // "   position: absolute; "
        // "   height: 0;        /* The div has no height */ "
        // "   line-height: 0;   /* The text line-box has no height */ "
        // "   margin: 0; "
        // "   padding: 0; "
        // "   overflow: visible; /* Let the text show, but the box is 'nothing' */ "
        // "} "

        "/* The Page Marker: A blue 2px line exactly at Top 0 */ "
        ".top-marker { "
        "    position: absolute; "
        "    top: 0; "
        "    left: 0; "
        "    width: 100%; "
        "    height: 2pt; "
        "    background: blue; "
        "} ",

        // "@media print { "
        // "    .page { "
        // "       border: none; "
        // "        box-shadow: none; "
        // "    } "
        // "    .subpage { "
        // "        border: 0.5mm solid green !important; "
        // "    } "
        // "} ",
        false
    );

    // head->new_node("style")->set_node_content(
    //     "/* Reset everything to zero */ "
    //     "* { "
    //     "    margin: 0; "
    //     "    padding: 0; "
    //     "    box-sizing: border-box; "
    //     "} "

    //     "@page { "
    //     "    size: 210mm 297mm; "
    //     "    margin: 0; "
    //     "} "

    //     ".page { "
    //     "    width: 210mm; "
    //     "    height: 296.6861mm !important;  "
    //     "    position: relative; "
    //     "    overflow: hidden; "
    //     "    background: white; "
    //     "    /* No border, no shadow, no padding */ "
    //     "} "

    //     ".subpage { "
    //     "    position: absolute; "
    //     "    width: 100%; "
    //     "    top: 50%; "
    //     "    left: 50%; "
    //     "    transform: translate(-50%, -50%); "
    //     "    height: 280.6861mm; "
    //     "    border: .5mm solid green; "
    //     "    /* This is our 'payload' box */ "
    //     "} "

    //     "/* The Grid: 1px red line every 10mm */ "
    //     ".grid-line { "
    //     "    position: absolute; "
    //     "    left: 0; "
    //     "    width: 100%; "
    //     "    height: 1px; "
    //     "    background: red; "
    //     "} "

    //     "/* The Page Marker: A blue 2px line exactly at Top 0 */ "
    //     ".top-marker { "
    //     "    position: absolute; "
    //     "    top: 0; "
    //     "    left: 0; "
    //     "    width: 100%; "
    //     "    height: 2px; "
    //     "    background: blue; "
    //     "} ",
    //     false
    //     );

    html_tree *body = dom.new_node("body");

    for (int p = 0; p != 2000; ++p) {
        html_tree *page = body->new_node("div class=\"page\"")->new_node("div class=\"subpage\"");
        page->new_node("div class=\"top-marker\"");
        for (int i = 0; i != 28; ++i) {
            page->new_node_f("div class=\"grid-line\" style=\"top: %.2fpt\"", (i + 1) * 28.25)->set_node_content_f("%dmm", (i + 1) * 10);
        }
        page->new_node("div class=\"page-number\"")->set_node_content_f("page %d", p + 1);
    }

    process_nodes(&dom);

    const char   *html = dom.get_html();
    std::ofstream file(std::filesystem::current_path().string() + "/A2-2000-measure_test.html");
    if (file) {
        file << html;
        file.close();
    }

    std::string baseURI = "file://" + std::filesystem::current_path().string() + "/";
    PDFprinter  pdf(baseURI.c_str());
    pdf.set_param(
        html,
        (std::filesystem::current_path().string() + "/A4-2000-measure_test.pdf").c_str()
    );
    pdf.layout("A4", "portrait");

    pdf.make_pdf();

    return 0;
}
