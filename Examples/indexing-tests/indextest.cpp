#include <filesystem>
#include <systemd/sd-journal.h>
#include <wk2gtkpdf/encode_image.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/index_pdf.h>
#include <wk2gtkpdf/pretty_html.h>

using namespace std;

WEBPAGE page() {
    WEBPAGE    html("<!DOCTYPE html>\n");
    html_tree  dom("html", html);
    html_tree *head = dom.new_node("head");
    head->new_node("style")
        ->set_node_content(
            "\n"
            ".index-item {\n"
            "    border: 1px solid #000;\n"
            "    margin: 10px 0;\n"
            "    display: block;\n"
            "}\n"

            ".index-item a {\n"
            "      color: blue;\n"
            "          text-decoration: none;\n"
            "}\n"

            ".index-item a:hover {\n"
            "      text-decoration: underline;\n"
            "}\n"

            ".anchor {\n"
            "    border: 1px solid #000;\n"
            "    padding: 5px;\n"
            "    margin: 10px 0;\n"
            "    display: block;\n"
            "}\n"
        );

    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait.css\"");

    html_tree *body = dom.new_node("body");
    html_tree *page = body->new_node("div class=\"page\"");
    page            = page->new_node("div class=\"subpage\"");
    page->new_node("h2")->set_node_content("Index Page");

    for (int i = 1; i != 11; ++i) {
        page
            ->new_node("div class=\"index-item\"")
            ->new_node("a href=\"#anchor" + std::to_string(i) + "\"")
            ->set_node_content("Anchor " + std::to_string(i));
    }

    int topMargin = 100;
    for (int i = 1; i != 11; ++i) {
        html_tree *page = body->new_node("div class=\"page\"");
        page            = page->new_node("div class=\"subpage\"");
        page
            ->new_node("h2")
            ->set_node_content("Page " + std::to_string(i + 1));
        page
            ->new_node("div class=\"anchor\" id=\"anchor" + std::to_string(i) + "\" style=\"margin-top:" + std::to_string(topMargin) + "px;\"")
            ->set_node_content("Anchor " + std::to_string(i));
        topMargin += 10;
    }

    pretty_html::process_nodes(&dom);

    return (html);
}

int main(int argc __attribute__((unused)), char **argv) {

    // SET UP LOGGING (all logging to journal)
    LOG_LEVEL = LOG_DEBUG;

    dup2(sd_journal_stream_fd(argv[0], LOG_LEVEL, 1), STDERR_FILENO);

    // INIT WEBKITGTK
    icGTK::init();

    WEBPAGE html = page();

    /**
     * @brief file
     *
     * Write the html to a plain html file (useful for testing)
     */
    std::ofstream file(std::filesystem::current_path().string() + "/testpage_anchor_only.html");
    if (file) {
        file << html;
        file.close();
    }
    {

        /**
         * @brief pdf
         *
         * Generate a pdf without an index
         */
        PDFprinter pdf;
        pdf.set_param(html, std::filesystem::current_path().string() + "/noindex.pdf");
        pdf.layout("A4", "portrait");
        pdf.make_pdf();
    }
    {
        /**
         * @brief pdf
         *
         * Do the same but add the anchors
         */
        PDFprinter pdf;
        pdf.set_param(html, std::filesystem::current_path().string() + "/indexed.pdf", true);
        pdf.layout("A4", "portrait");
        pdf.make_pdf();
    }
}
