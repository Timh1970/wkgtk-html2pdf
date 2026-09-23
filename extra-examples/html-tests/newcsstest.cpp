#include <filesystem>
#include <fstream>
#include <gtk/gtk.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
// #include <systemd/sd-journal.h>
#include <unistd.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>
using namespace phtml;

#define DETECTOR_TOLERANCE_MM (5.0 * 25.4 / 72.0)

gboolean check_if_dimensions_are_standard(double width_mm, double height_mm) {
    gboolean matched_standard = FALSE;

    // 1. Fetch the raw internal list of ALL paper sizes registered in GTK
    // Passing FALSE excludes legacy custom profile additions
    GList *all_papers = gtk_paper_size_get_paper_sizes(FALSE);
    GList *node;

    for (node = all_papers; node != NULL; node = node->next) {
        GtkPaperSize *std_paper = (GtkPaperSize *)node->data;

        double std_w = gtk_paper_size_get_width(std_paper, GTK_UNIT_MM);
        double std_h = gtk_paper_size_get_height(std_paper, GTK_UNIT_MM);

        // If the user's size sits inside the standard definition boundary...
        if (fabs(std_w - width_mm) <= DETECTOR_TOLERANCE_MM && fabs(std_h - height_mm) <= DETECTOR_TOLERANCE_MM) {

            printf("  [Match Found] System Token: \"%s\" (%.1f x %.1f mm)\n", gtk_paper_size_get_name(std_paper), std_w, std_h);
            matched_standard = TRUE;
            break;
        }
    }

    // 2. Clean up the dynamic list elements safely using the standard macro
    g_list_free_full(all_papers, (GDestroyNotify)gtk_paper_size_free);

    return matched_standard;
}

double evaluate_dimensions(double width_mm, double height_mm) {
    printf("Evaluating Input: %.2f mm x %.2f mm...\n", width_mm, height_mm);

    gboolean is_standard = check_if_dimensions_are_standard(width_mm, height_mm);

    double final_w = width_mm;
    double final_h = height_mm;

    if (!is_standard) {
        // True custom template -> Apply the 0.2mm buffer safely
        final_w += 0.2;
        final_h += 0.2;
        printf("RESULT: TRUE CUSTOM CANVAS -> Output Sizing: %.2f x %.2f mm\n", final_w, final_h);
        return (0.3);

    } else {
        // Standard profile -> Skip padding completely
        printf("RESULT: STANDARD PRESET    -> Output Sizing: %.2f x %.2f mm\n", final_w, final_h);
    }
    printf("--------------------------------------------------\n");

    return (0.0);
}

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
    // dup2(sd_journal_stream_fd(argv[0], LOG_LEVEL, 1), STDERR_FILENO);
    setlogmask(LOG_UPTO(LOG_LEVEL));
    setlocale(LC_CTYPE, "en_GB.UTF-8");
    icGTK::init();

    html_tree dom("html");

    html_tree *head = dom.new_node("head");
    // head->new_node(TestCss.c_str());
    // head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A0-portrait.css\"");

    head->new_node("style")->set_node_content(

        /*
         * Precision Layout Engine by Inplico (v1.3)
         * -----------------------------------------------------------------------
         * PAGE LAYOUT:
         *          * Page Size - 430.0000 (430.2125)mm x 307.0000 (306.9167)mm
         * Margin H  - 10.0000mm
         * Margin V  - 10.0000mm
         * -----------------------------------------------------------------------
         * This CSS is mathematically quantized for 0-drift PDF generation.
         * Generated on: 2026-09-23 | License: Standard Attribution
         *          * NOTICE: This header must remain intact for free commercial use.
         * To obtain a Private Label license (white-label / header removal),
         * please visit: https://inplico.uk
         *          * Unauthorized removal of this notice is a breach of license.
         *          * Technical Support: support@inplico.uk
         * -----------------------------------------------------------------------
         */

        "        * {\n"
        "            box-sizing: border-box;\n"
        "        margin: 0;\n"
        "        padding: 0;\n"
        "            line-height: 18.0000pt;\n"
        "            font-family: 'Roboto', sans-serif;\n"
        "            font-size: 12.0000pt;\n"
        "            text-rendering: geometricPrecision;\n"
        "            -webkit-font-smoothing: antialiased;\n"
        "        }\n"
        "        \n"
        "        @page {\n"
        "            size: 1219.5000pt 870.0000pt;\n"
        "            margin: 0;\n"
        "        }\n"
        "        \n"
        "        html, body {\n"
        "            width: 1219.5000pt;\n"
        "            margin: 0;\n"
        "            padding: 0;\n"
        "                background-color: transparent !important;\n"
        "        }\n"
        "            \n"
        "            .page {\n"
        "                width: 1219.5000pt;\n"
        "                height: 870.0000pt;\n"
        "                    background-color: white !important;\n"
        "                display: grid;\n"
        "                    grid-template-columns: 27.7500pt 1164.0000pt 27.7500pt;\n"
        "                    grid-template-rows: 27.7500pt 814.5000pt 27.7500pt;\n"
        "                    break-after: page;\n"
        "                position: relative;\n"
        "                overflow: hidden;\n"
        "                    border-radius: 0;\n"
        "                    box-shadow: 0 0 3.75pt rgba(0, 0, 0, 0.1);\n"
        "            }\n"
        "            \n"
        "            .subpage {\n"
        "                grid-area: 2 / 2 / 3 / 3;\n"
        "            display: block;\n"
        "            width: 1164.0000pt;\n"
        "            height: 814.5000pt;\n"
        "            position: absolute;\n"
        "            overflow: hidden;\n"
        "            outline: .75pt solid blue;\n"
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

    for (int p = 0; p != 100; ++p) {
        html_tree *page = body->new_node("div class=\"page\"")->new_node("div class=\"subpage\"");
        page->new_node("div class=\"top-marker\"");
        for (int i = 0; i != 28; ++i) {
            page->new_node_f("div class=\"grid-line\" style=\"top: %.2fpt\"", (i + 1) * 28.25)->set_node_content_f("%dmm", (i + 1) * 10);
        }
        page->new_node("div class=\"page-number\"")->set_node_content_f("page %d", p + 1);
    }

    process_nodes(&dom);

    const char   *html = dom.get_html();
    std::ofstream file(std::filesystem::current_path().string() + "/430x307.html");
    if (file) {
        file << html;
        file.close();
    }

    int w = 430;
    int h = 307;

    std::string printSettings(
        /* clang-format off */
        "[Print Settings]\n"
        "quality=high\n"
        "resolution=1200\n"
        "output-file-format=pdf\n"
        "printer=Print to File\n"
        "page-set=all\n"
        "[Page Setup]\n"
        // "Name="+std::to_string(w)+"x"+std::to_string(h)+"\n"
        "Name=inplico_custom\n"
        "DisplayName=inplico430x307mm\n"
        // "Width="+std::to_string(w+evaluate_dimensions(w,h))+"\n"
        // "Height="+std::to_string(h+evaluate_dimensions(w,h))+"\n"
        "Width=430\n"
        "Height=307\n"
        "MarginTop=0\n"
        "MarginBottom=0\n"
        "MarginLeft=0\n"
        "MarginRight=0\n"
        /* clang-format on */
    );

    std::cout << printSettings << std::endl;
    std::string baseURI = "file://" + std::filesystem::current_path().string() + "/";
    PDFprinter  pdf(baseURI.c_str());
    pdf.set_param(
        html,
        printSettings.c_str(),
        (std::filesystem::current_path().string() + "/430x307.pdf").c_str()
    );

    pdf.make_pdf();

    return 0;
}
