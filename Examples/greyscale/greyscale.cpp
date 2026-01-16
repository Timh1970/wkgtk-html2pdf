#include <filesystem>
#include <fstream>
#include <wk2gtkpdf/encode_image.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>
#define MAGICKCORE_HDRI_ENABLE 1
#include <Magick++.h>

using namespace std;

WEBPAGE make_page() {

    string    html("<DOCTYPE html>");
    html_tree dom("html", html);

    html_tree *head = dom.new_node("head");
    // IMPORT PAGE LAYOUT
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A5-portrait.css\"");
    html_tree *style = head->new_node("style");

    // ADD SOME STYLING
    style->set_node_content(
        R"(img{
    display: flex;
    width: 100%;
    height: 100%;
})"
    );

    // CREATE THE PAGE
    html_tree *elem = dom.new_node("body");
    elem            = elem->new_node("div class=\"page\"");
    elem            = elem->new_node("div class=\"subpage\"");

    // IMAGE
    encode_image img("./example-logo.png");
    elem->new_node("img src=" + img.b64_image());

    pretty_html::process_nodes(&dom);

    return (html);
}

int main() {

    // SET UP LOGGING (all logging to journal)
    LOG_LEVEL = LOG_DEBUG;
    // INIT WEBKITGTK
    icGTK::init();

    WEBPAGE       html = make_page();
    /**
     * @brief file
     *
     * Write the html to a plain html file (useful for testing)
     */
    std::ofstream file(std::filesystem::current_path().string() + "/output_color.html");
    if (file) {
        file << html;
        file.close();
    }

    /**
     * @brief pdf
     *
     * Output a colour version of the html
     */
    PDFprinter pdf;
    pdf.set_param(html, std::filesystem::current_path().string() + "/output_color.pdf");
    pdf.layout("A5", "portrait");
    pdf.make_pdf();

    /**
     * @brief pdfBLB
     *
     * Post process with ImageMagick to make greyscale.
     */
    PDFprinter pdfBLB;
    pdfBLB.set_param(html);
    pdfBLB.layout("A5", "portrait");
    pdfBLB.make_pdf();

    PDFprinter::blob pdBLOB = pdfBLB.get_blob();
    Magick::Blob     blb(pdBLOB.data(), pdBLOB.size());
    Magick::Image    img;
    img.magick("PDF");
    img.read(blb);
    img.type(Magick::GrayscaleType);
    img.write(std::filesystem::current_path().string() + "/output_gs.pdf");
}
