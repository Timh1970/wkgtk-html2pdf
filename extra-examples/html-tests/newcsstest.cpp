#include <filesystem>
#include <fstream>
#include <systemd/sd-journal.h>
#include <unistd.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>

using namespace phtml;

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 *
 * This function is used for testing changes to the main page template in order
 * to ensure that they do not introduce drift.
 *
 * I recommend doing a 2000 page A4 and a 200 page A0
 */
int main(int argc, char *argv[]) {

    std::string TestCss = "link rel=\"stylesheet\" href=\"" + std::filesystem::current_path().string() + "/A0-portrait.css\"";
    LOG_LEVEL           = LOG_WARNING;
    // REDIRECT WEBKIT LOGGING TO SYSLOG
    dup2(sd_journal_stream_fd(argv[0], LOG_LEVEL, 1), STDERR_FILENO);
    setlogmask(LOG_UPTO(LOG_LEVEL));
    setlocale(LC_CTYPE, "en_GB.UTF-8");
    icGTK::init();

    html_tree dom("html");

    html_tree *head = dom.new_node("head");
    // head->new_node(TestCss.c_str());
    // head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A0-portrait.css\"");

    head->new_node("style")->set_node_content(

        "        /*\n"
        "         * Precision Layout Engine by Inplico (v2.0-Skia)\n"
        "         * -----------------------------------------------------------------------\n"
        "         * PAGE LAYOUT:\n"
        "         *          * Page Size         - 841.0000 (840.8458)mm x 1189.0000 (1188.7729)mm\n"
        "         * Target Dimensions - 3178.0000px x 4493.0000px\n"
        "         * Margin H          - 8.0000mm (30.0000px)\n"
        "         * Margin V          - 8.0000mm (30.0000px)\n"
        "         * -----------------------------------------------------------------------\n"
        "         * This CSS is mathematically quantized for 0-drift PDF generation.\n"
        "         * Generated on: 2026-07-24 | License: Standard Attribution\n"
        "         *          * NOTICE: This header must remain intact for free commercial use.\n"
        "         * To obtain a Private Label license (white-label / header removal),\n"
        "         * please visit: https://inplico.uk\n"
        "         *          * Technical Support: support@inplico.uk\n"
        "         * -----------------------------------------------------------------------\n"
        "         */\n"
        "        \n"
        "        * {\n"
        "            box-sizing: border-box;\n"
        "        margin: 0;\n"
        "        padding: 0;\n"
        "            line-height: 24px;\n"
        "            font-family: 'Liberation Sans', sans-serif;\n"
        "            font-size: 16px;\n"
        "        }\n"
        "        \n"
        "        @page {\n"
        "            size: 3178.0000px 4493.0000px;\n"
        "            margin: 0;\n"
        "        }\n"
        "        \n"
        "        html, body {\n"
        "            width: 3178.0000px;\n"
        "            margin: 0;\n"
        "            padding: 0;\n"
        "                background-color: transparent !important;\n"
        "        }\n"
        "            \n"
        "            .page {\n"
        "                width: 3178.0000px;\n"
        "                height: 4493.0000px;\n"
        "                    background-color: white !important;\n"
        "                display: grid;\n"
        "                    /* Using 1fr prevents edge overflow bugs during layout engine grid resolution */\n"
        "                    grid-template-columns: 30.0000px 1fr 30.0000px;\n"
        "                    grid-template-rows: 30.0000px 1fr 30.0000px;\n"
        "                    break-after: page;\n"
        "                position: relative;\n"
        "                overflow: hidden;\n"
        "                    border-radius: 0;\n"
        "                    box-shadow: 0 0 5px rgba(0, 0, 0, 0.1);\n"
        "            }\n"
        "            \n"
        "            .subpage {\n"
        "                grid-area: 2 / 2 / 3 / 3;\n"
        "            display: block;\n"
        "            position: relative;\n"
        "            overflow: hidden;\n"
        "            outline: 1px solid blue;\n"
        "            }\n"
        "        \n"
        "        @media print {\n"
        "            .page {\n"
        "            border: none;\n"
        "                box-shadow: none;\n"
        "            }\n"
        "            \n"
        "            .page:last-of-type {\n"
        "                break-after: avoid !important;\n"
        "                page-break-after: avoid !important;\n"
        "            }\n"
        "            \n"
        "            .subpage { outline: none; }\n"
        "        }\n"

        /* The Grid: 1pt red line every 10mm */
        ".grid-line { "
        "    position: absolute; "
        "    left: 0; "
        "    width: 100%; "
        "    height: .75pt; "
        "    background: red; "
        "} "

        /* The Page Marker: A blue 2pt line exactly at Top 0 */
        ".top-marker { "
        "    position: absolute; "
        "    top: 0; "
        "    left: 0; "
        "    width: 100%; "
        "    height: 2.25pt; "
        "    background: blue; "
        "} ",

        false
    );

    html_tree *body = dom.new_node("body");

    for (int p = 0; p != 250; ++p) {
        html_tree *page = body->new_node("div class=\"page\"")->new_node("div class=\"subpage\"");
        page->new_node("div class=\"top-marker\"");
        for (int i = 0; i != 119; ++i) {
            page->new_node_f("div class=\"grid-line\" style=\"top: %.2fpt\"", (i + 1) * 28.25)->set_node_content_f("%dmm", (i + 1) * 10);
        }
        page->new_node("div class=\"page-number\"")->set_node_content_f("page %d", p + 1);
    }

    process_nodes(&dom);

    const char   *html = dom.get_html();
    std::ofstream file(std::filesystem::current_path().string() + "/newcsstestA0.html");
    if (file) {
        file << html;
        file.close();
    }

    std::string printSettings(
        /* clang-format off */
            "[Print Settings]\n"
            "quality=high\n"
            "resolution=96\n"
            "output-file-format=pdf\n"
            "printer=Print to File\n"
            "page-set=all\n"
            "[Page Setup]\n"
            "PPDName=inplico\n"
            "DisplayName=inplicoa0\n"
            "Width=841\n"
            "Height=1189\n"
            "MarginTop=0\n"
            "MarginBottom=0\n"
            "MarginLeft=0\n"
            "MarginRight=0\n"
            "Orientation=portrait\n"
        /* clang-format on */
    );

    std::string baseURI = "file://" + std::filesystem::current_path().string() + "/";
    PDFprinter  pdf(baseURI.c_str());
    pdf.set_param(
        html,
        printSettings.c_str(),
        (std::filesystem::current_path().string() + "/newcsstestA0.pdf").c_str()
    );

    pdf.make_pdf();

    return 0;
}
