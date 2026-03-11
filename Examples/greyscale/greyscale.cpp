#include <cstring>
#include <filesystem>
#include <fstream>
#include <wk2gtkpdf/encode_image.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>
#ifndef MAGICKCORE_HDRI_ENABLE
#define MAGICKCORE_HDRI_ENABLE 1
#endif
#include <Magick++.h>

using namespace std;
using namespace phtml;

const char *make_page() {

    html_tree dom("html");

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

    // LOGO
    /**
     * @brief img
     * This example grabs an image and base64 encodes it;
     * it is not at all necessary to do so, this is just
     * one way; you could always link the image:
     *
     *  ->new_node("img src=\"example-logo.png\");
     *
     * if you wished as long as you set the relative path
     * correctly when instantiating a PDFprinter so that
     * PDFprinter knows where to find it.
     *
     * Another alternative is to use a full path
     *
     *  ->new_node("img src=\"/foo/bar/example-logo.png\");
     *
     * If you are utilising the relative path for another
     * purpose (eg. to link your stylesheets)
     *
     * @warning If you are going to pass raw image data
     * this way then do not use the helper function
     * new_node_f() with std::string()  If you wish to
     * to concat a std::string() you must use
     *
     *  std::string().c_str()
     *
     * eg.
     *
     *  new_node_f("img src=%s", myimg.c_str())
     *
     * Technical reasons: std::string has an unstable
     * ABI across different compilers/versions. Always
     * use .c_str() with variadic helpers to ensure
     * binary compatibility.
     *
     * Alternatively use the method below.
     */
    encode_image img("example-logo.png");
    const char  *encImg = img.b64_image();
    const char  *prefix = "img src=";

    size_t lenPrefix = strlen(prefix);
    size_t lenImg    = strlen(encImg);
    char  *strImg    = (char *)malloc(lenPrefix + lenImg + 1);

    if (strImg) {
        memcpy(strImg, prefix, lenPrefix);
        memcpy(strImg + lenPrefix, encImg, lenImg + 1);
    }
    elem->new_node(strImg);
    /**
     * IMPORTANT: new_node copies the string internally,
     * so you MUST free your temporary malloc'd buffer here.
     * */
    free(strImg);

    process_nodes(&dom);

    // GET THE GENERATED HTML
    const char *html = dom.get_html();

    // MAKE A LOCAL COPY TO RETURN IT TO THE CALLER
    const char *buf = strdup(html);
    return (buf);
}

int main() {

    // SET UP LOGGING (all logging to journal)
    LOG_LEVEL = LOG_DEBUG;
    // INIT WEBKITGTK
    icGTK::init();

    const char   *html = make_page();
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
     * Output a colour version of the html for verification
     */
    PDFprinter pdf;
    pdf.set_param(html, (std::filesystem::current_path().string() + "/output_color.pdf").c_str());
    pdf.layout("A5", "portrait");
    pdf.make_pdf();

    /**
     * @brief pdfBLB
     *
     * Post process with ImageMagick to make grayscale.
     */
    PDFprinter pdfBLB;

    /**
     * By not providing an output file we are implicitly
     * telling PDFprinter we want a BLOB (Binary Large
     * Object)
     * */
    pdfBLB.set_param(html);
    pdfBLB.layout("A5", "portrait");
    pdfBLB.make_pdf();

    /**
     * @brief binDat
     *
     * Get the PDF from PDFprinter
     */
    PDF_Blob binDat = pdfBLB.get_blob();

    /**
     * @brief blb
     *
     * Hand it to image magick
     */
    Magick::Blob blb(binDat.data, binDat.size);

    /**
     * @brief img
     *
     * Convert it to grayscale with image magick
     */
    Magick::Image img;
    img.magick("PDF");
    img.read(blb);
    img.type(Magick::GrayscaleType);
    img.write(std::filesystem::current_path().string() + "/output_gs.pdf");

    /**
     * Clean up.
     */
    PDF_FreeBlob(binDat);
    PDF_FreeHTML(html);
}
