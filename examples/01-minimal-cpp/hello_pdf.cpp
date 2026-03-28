#include <filesystem>
#include <fstream>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/pretty_html.h>

using namespace phtml;

/**
 * @brief main
 * @return
 *
 * This demo is the simplest example of generating some html and turning
 * it into a pdf.
 *
 * If you wish to simply pass in a file then see the source code for the
 * command line interface.
 */
int main() {

    /**
     * @brief icGTK::init
     *
     * Set up the gtk environment to enable printing.
     *
     * This class is a singleton; it only needs to be called once;
     * it will stay resident for the duration of the application
     * and automatically clean itself up at the end.
     *
     * It is safe, albeit pointless, to call it multiple times as
     * it will simply return the same instance.
     *
     * @note It is not needed if all you want to do is generate
     * html; it is only used for converting html to pdf. Pretty_html
     * does not need to be initialised.
     */
    icGTK::init();

    /**
     * @brief dom
     *
     * The built in pretty_html functions enable you to generate
     * html easily without having to worry about opening and and
     * closing elements (which is done automatically.
     *
     * This is the primary element <html> and here is where we
     * pass in the webpage string.
     */
    phtml::html_tree dom("html");

    /**
     * @brief head
     * A nested head element.
     */
    phtml::html_tree *head = dom.new_node("head");

    /**
     * An absolute link to one of the base stylesheets used to
     * format the page.
     */
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait.css\"");

    /**
     * @brief body
     * The body element.
     */
    phtml::html_tree *body = dom.new_node("body");

    /**
     * @brief page
     *
     * Create a page.
     *
     * @note that the first declaration is for the page itself and the
     * second is the area inside the page margin. We create a separate
     * reference to the innermost element for adding content.
     */
    phtml::html_tree *page    = body->new_node("div class=\"page\"");
    phtml::html_tree *subpage = page->new_node("div class=\"subpage\"");

    /**
     * Put something on the page.
     */
    subpage->new_node("h1")->set_node_content("Hello World");
    subpage->new_node("p")->set_node_content(
        "Testing PDF generation; for a more complete example see <i>demo-form</i> in the examples folder."
    );

    /**
     * compile the html
     */
    phtml::process_nodes(&dom);

    const char   *html = dom.get_html();
    /**
     * @brief file
     *
     * Write the html to a plain html file (useful for testing)
     */
    std::ofstream file(std::filesystem::current_path().string() + "/hellopdf.html");
    if (file) {
        file << html;
        file.close();
    }

    /**
     * @brief pdf
     *
     * The PDFprinter class is what you use to generate a pdf
     * from html.
     */
    PDFprinter pdf;

    /**
     * The basic parameters required to generate a pdf are the
     * html itself, a page configuration string, and a filename
     * for the output.
     */
    pdf.set_param(
        html,
        (std::filesystem::current_path().string() + "/hello.pdf").c_str()
    );

    /**
     * Once you have set the payload you can generate the pdf.
     */
    pdf.make_pdf();

    return (0);
}
