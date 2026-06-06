#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <wk2gtkpdf/encode_image.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>

using namespace phtml;

void contact_table(html_tree *formArea) {

    html_tree *table = formArea->new_node("table class=\"fixed\"");
    table->new_node("col width=\"80.25pt\"");
    table->new_node("col width=\"209.25pt\"");
    table->new_node("col width=\"209.25pt\"");

    // TABLE HEADING
    html_tree *row = table->new_node("tr");
    row->new_node("th scope=\"colgroup\" colspan=\"3\"")
        ->set_node_content("Call Details");

    row = table->new_node("tr");
    row->new_node(
        "td class=\"label\" style=\"border-bottom: none; border-right: none;\""
    );
    row->new_node(
           "td class=\"label\" style=\"border-left: none; border-right: none;\""
    )
        ->set_node_content("Customer:");
    row->new_node("td class=\"label\" style=\"border-left: none;\"")
        ->set_node_content("Site:");
    // CUSTOMER NAME
    row = table->new_node("tr");
    row->new_node(
           "td class=\"label\" style=\"border-top: none; border-bottom: none;\""
    )
        ->set_node_content("Name:");
    row->new_node("td class=\"field\"")
        ->set_node_content("John Doe");

    // SITE NAME
    row->new_node("td class=\"field\"")
        ->set_node_content("Banana headquarters");

    // CUSTOMER ADDRESS
    row = table->new_node("tr");
    row->new_node(
           "td class=\"label\" style=\"border-top: none; border-bottom: none;\""
    )
        ->set_node_content("Address:");

    /**
     * @brief sAddress
     * For the sake of this sample form the height set here is simply the field height in the css * 4
     * In production you would probably want to set an explicit style or do it with a CSS variable and
     * a multiplier to keep the field a static height but relative to any changed.
     */
    html_tree *cAddress = row->new_node("td class=\"field\" style=\"height: 68.25pt; vertical-align: top;\"");
    cAddress->set_node_content("31 Spooner Street");
    cAddress->new_node("br")->set_node_content("Quahog");
    cAddress->new_node("br")->set_node_content("Cranston");
    cAddress->new_node("br")->set_node_content("Rhode Island");

    // SITE ADDRESS
    html_tree *sAddress = row->new_node("td class=\"field\" style=\"height: 68.25pt; vertical-align: top;\"");
    sAddress->set_node_content("1407 Greymalkin Lane");
    sAddress->new_node("br")->set_node_content("Salem Center");
    sAddress->new_node("br")->set_node_content("New York");
    sAddress->new_node("br")->set_node_content("ABC 123");

    // CUSTOMER TEL
    row = table->new_node("tr");
    row->new_node("td class=\"label\" style=\"border-top: none;\"")
        ->set_node_content("Contact Tel:");
    row->new_node("td class=\"field\"")
        ->set_node_content("0898 221333");

    // SITE TEL
    row->new_node("td class=\"field\"")
        ->set_node_content("9262 458787");
}

void build_call_table(html_tree *formArea) {

    html_tree *table = formArea->new_node("table class=\"fixed\"");
    table->new_node("col width=\"80.25pt\"");
    table->new_node("col width=\"99.75pt\"");
    table->new_node("col width=\"80.25pt\"");
    table->new_node("col width=\"239.25pt\"");

    html_tree *row = table->new_node("tr");

    // 1ST ROW
    // DATE
    row->new_node("td class=\"label\" style=\"border-bottom: none;\"")
        ->set_node_content("C.order No.");
    row->new_node("td class=\"field\"")
        ->set_node_content("12345");
    // MACHINE
    row->new_node("td class=\"label\" style=\"border-bottom: none;\"")
        ->set_node_content("Machine:");
    row->new_node("td class=\"field\"")
        ->set_node_content("SAMPLE");

    // 3RD ROW
    row = table->new_node("tr");
    // ENGINEER
    row->new_node("td class=\"label\" style=\"border-top: none;\"")
        ->set_node_content("Engineer:");
    row->new_node("td class=\"field\""); // Engineer field information goes here
                                         // once implemented
    // METER
    row->new_node("td class=\"label\" style=\"border-top: none;\"")
        ->set_node_content("Meter:");
    row->new_node("td class=\"field\"")
        ->set_node_content("3216549");
}

void build_faults_table(html_tree *formArea) {
    // FAULT TABLE HEADERS
    html_tree *table = formArea->new_node("table class=\"fixed\"");
    table->new_node("col width=\"80.25pt\"");
    table->new_node("col width=\"339pt\"");
    table->new_node("col width=\"80.25pt\"");
    table->new_node("tr")
        ->new_node("th colspan=\"3\"")
        ->set_node_content("Faults");

    html_tree *row = table->new_node("tr");
    row->new_node(
        "td class=\"label\" style=\"border-right: none; border-bottom: "
        "none;\" colspan=\"2\""
    );
    row->new_node("td class=\"label\" style=\"border-left: none;\"")
        ->set_node_content("Status:");

    for (int i = 0; i != 4; ++i) {
        row = table->new_node("tr");
        // DESCRIPTION LABEL
        row->new_node(
               "td class=\"label\" style=\"border-top: none; border-bottom: none;\""
        )
            ->set_node_content("Description:");
        // DESCRIPTION FIELD
        row->new_node("td class=faultfield");
        // STATUS FIELD
        row->new_node("td class=faultfield rowspan=\"2\"");

        row = table->new_node("tr");
        // NOTES LABEL
        row->new_node("td class=\"label\" style=\"border-top: none;\"")
            ->set_node_content("Notes:");
        row->new_node("td class=faultfield");
    }
}

void build_parts_table(html_tree *formArea) {
    html_tree *table = formArea->new_node("table class=\"fixed\"");
    table->new_node("col width=\"39.75pt\"");
    table->new_node("col width=\"120pt\"");
    table->new_node("col width=\"339pt\"");
    // PARTS TABLE HEADER
    table->new_node("tr")
        ->new_node("th colspan=\"3\"")
        ->set_node_content("Parts");

    // COLUMN HEADERS
    html_tree *row = table->new_node("tr");
    row->new_node("td class=\"label\" style=\"border-right: none;\"")
        ->set_node_content("Qty:");
    row->new_node(
           "td class=\"label\" style=\"border-left: none; border-right: none;\""
    )
        ->set_node_content("Part No.");
    row->new_node("td class=\"label\" style=\"border-left: none;\"")
        ->set_node_content("Description:");

    // FIELDS
    for (int i = 0; i != 4; ++i) {
        row = table->new_node("tr");
        // QUANTITY
        row->new_node("td class=\"field\"")
            ->set_node_content("1");
        // PARTNO
        html_tree *partNo = row->new_node("td class=\"field\"");
        partNo->set_node_content("1234567");
        // DESCRIPTION
        row->new_node("td class=\"field\"")
            ->set_node_content("SAMPLE");
    }
}

void build_labour_table(html_tree *formArea) {

    html_tree *table = formArea->new_node("table class=\"fixed\"");
    table->new_node("col width=\"99.75pt\"");
    table->new_node("col width=\"162pt\"");
    table->new_node("col width=\"162pt\"");
    table->new_node("col width=\"75pt\"");
    // LABOUR TABLE HEADER
    table->new_node("tr")
        ->new_node("th colspan=\"4\"")
        ->set_node_content("Labour");

    // COLUMN HEADERS
    html_tree *row = table->new_node("tr");
    row->new_node("td class=\"label\" style=\"border-right: none;\"")
        ->set_node_content("Engineer:");
    row->new_node(
           "td class=\"label\" style=\"border-left: none; border-right: none;\""
    )
        ->set_node_content("Start");
    row->new_node(
           "td class=\"label\" style=\"border-left: none; border-right: none;\""
    )
        ->set_node_content("Finish");
    row->new_node("td class=\"label\" style=\"border-left: none;\"")
        ->set_node_content("Total:");

    for (int i = 0; i != 4; ++i) {
        row = table->new_node("tr");
        for (int i = 0; i != 4; ++i) {
            row->new_node("td class=\"field\"");
        }
    }

    row = table->new_node("tr");
    row->new_node("td class=\"label\" colspan=3 style=\"text-align: right;\"")
        ->set_node_content("G.Total:");
    row->new_node("td class=\"field\"")
        ->set_node_content("0h:0m");
}

void build_travel_table(html_tree *formArea) {

    html_tree *table = formArea->new_node("table class=\"fixed\"");
    table->new_node("col width=\"199.5pt\"");
    table->new_node("col width=\"199.5pt\"");
    table->new_node("col width=\"50.25pt\"");
    table->new_node("col width=\"50.25pt\"");

    // PARTS TABLE HEADER
    table->new_node("tr")
        ->new_node("th colspan=\"4\"")
        ->set_node_content("Travel");

    // COLUMN HEADERS
    html_tree *row = table->new_node("tr");
    row->new_node("td class=\"label\" style=\"border-right: none;\"")
        ->set_node_content("Depart:");
    row->new_node(
           "td class=\"label\" style=\"border-left: none; border-right: none;\""
    )
        ->set_node_content("Arrive:");

    row->new_node(
           "td class=\"label\" style=\"border-left: none; "
           "border-right: none;\""
    )
        ->set_node_content("Miles:");
    row->new_node("td class=\"label\" style=\"border-left: none;\"")
        ->set_node_content("Time:");

    for (int i = 0; i != 4; ++i) {

        row = table->new_node("tr");
        row->new_node("td class=\"field\"");
        row->new_node("td class=\"field\"");
        row->new_node("td class=\"field\" style=\"text-align:right\"");
        row->new_node("td class=\"field\"");
    }

    // TOTALS
    row = table->new_node("tr");
    row->new_node("td class=\"label\" colspan=2 style=\"text-align: right;\"")
        ->set_node_content("Totals:");
    row->new_node("td class=\"field\" style=\"text-align:right\"");
    row->new_node("td class=\"field\"");
}

void info_page(html_tree *subpage) {

    subpage->new_node("h2")->set_node_content("About");
    subpage->new_node("p")->set_node_content(
        "The demo form has been constructed using the built in pretty_html "
        "feature which aids in the construction of html using a built in "
        "class that automatically handles the opening and closing of html "
        "elements."
    );

    subpage->new_node("p")->set_node_content(
        "It can be compiled using the makefile in this folder.  When run it will "
        "automatically generate this page in html and the pdf of the same name "
        "in this folder."
    );

    subpage->new_node("h2")->set_node_content("Margins");
    subpage->new_node("p")->set_node_content(
        "Page margins are defined in the bundled templates. While the defaults "
        "follow standard technical documentation guidelines, business forms "
        "often require narrower margins to maximize layout real estate. You can "
        "modify the templates directly, but we recommend creating a custom copy "
        "to preserve the original defaults. "

    );

    subpage->new_node("h2")->set_node_content("Avoid Double Scaling");
    subpage->new_node("p")->set_node_content(
        "The most common cause of incorrect margin sizes is printer-side scaling. "
        "Because these templates already define precise hardware margins, applying "
        "'Fit to Page' or 'Shrink to Fit' in your PDF viewer will compound the "
        "margins, resulting in an undersized layout. For 1:1 parity, always print "
        "at 'Actual Size' (100%)."
    );

    subpage->new_node("h2")->set_node_content("Style Sheets");

    subpage->new_node("p")->set_node_content(
        "This example embeds the stylesheet directly. While not required, this ensures "
        "the HTML remains completely portable and self-contained, preventing layout "
        "breakage if external CSS assets are moved or unreachable. "
    );

    subpage->new_node("h2")->set_node_content("Images");
    subpage->new_node("p")->set_node_content(
        "To ensure document integrity and total portability, the logo in this example is "
        "Base64 encoded and embedded directly. This eliminates external dependencies and "
        "prevents 'broken image' placeholders in air-gapped or archived environments."
    );

    // PAGES
    subpage->new_node("h2")->set_node_content("Pages");
    subpage->new_node("p")->set_node_content(
        "New pages are created by utilising the<b> page</b> and<b> subpage</b> classes."
    );

    // THE PAGES CODE DEMO
    html_tree *divpg = subpage->new_node("div class=\"nested-code\"");
    divpg->set_node_content("&lt;div class=\"page\"&gt;");
    html_tree *divsub = divpg->new_node("div class=\"nested-code\"");
    divsub->set_node_content("&lt;div class=\"subpage\"&gt;");
    divsub->new_node("div class=\"nested-code\"")->set_node_content("page content....<br>");
    subpage->new_node("div class=\"nested-code\"")
        ->new_node("div class=\"nested-code\"")
        ->set_node_content("&lt;/div&gt;");
    subpage->new_node("div class=\"nested-code\"")
        ->set_node_content("&lt;/div&gt;");
}

const char *webpage() {

    wkJlog << iclog::loglevel::debug << iclog::category::CLI
           << "Constructing html form."
           << iclog::endl;

    // PRIMARY ELEMENT
    html_tree dom("html");

    // HEAD
    html_tree *head = dom.new_node("head");
    head->new_node("meta charset=\"UTF-8\"");
    head->new_node("meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"");

    // STYLE [DEPRICATED]
    /**
     * Built in style for an ISO A4 page (see templates folder)
     * */
    // head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait-lite.css\"");

    /**
     * @brief f
     *
     * Embed the style here; it doesn't need to be embedded if
     * it is in an accessible location.
     */
    std::ifstream f("demo-form.css");
    std::string   style((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    head->new_node("style")->set_node_content(style.c_str());

    // BODY
    html_tree *body    = dom.new_node("body");
    // PAGE
    html_tree *page    = body->new_node("div class=\"page\"");
    html_tree *subpage = page->new_node("div class=\"subpage\"");

    /**
     * @brief container
     *
     * While <table> handles dynamic data well, we use display: grid for the high-level
     * document structure (Heading vs. Form). This ensures that the vertical 'anchor'
     * points for each section are locked to the integer-point grid, preventing the header
     * from 'pushing' the form into a fractional-pixel drift.
     *
     * This is the grid container (we only use rows and one column in this demo)
     */
    html_tree *container = subpage->new_node("div class=\"container\"");

    /**
     * @brief headingArea
     *
     * The grid area reserved for the heading
     */
    html_tree *headingArea = container->new_node("div class=\"heading\"");

    /**
     * @brief img
     *
     * It is not necessary to base64 encode an image, but can prove useful if you wish to
     * email the HTML.
     */
    encode_image img("example-logo.png");
    const char  *encImg = img.b64_image();
    const char  *prefix = "img class=\"logo\" src=";

    size_t lenPrefix = strlen(prefix);
    size_t lenImg    = strlen(encImg);
    char  *strImg    = (char *)malloc(lenPrefix + lenImg + 1);

    if (strImg) {
        memcpy(strImg, prefix, lenPrefix);
        memcpy(strImg + lenPrefix, encImg, lenImg + 1);
    }

    headingArea->new_node(strImg);

    free(strImg);

    // std::cout << strImg << std::endl;
    // TITLE
    headingArea->new_node("h1 class=\"doctitle\"")->set_node_content("DEMO Job sheet");

    // REFERENCE
    container->new_node("h4 class=\"ref\"")
        ->set_node_content("Ref: 1234/<br>20260111");

    /**
     * @brief formArea
     *
     * The grid area reserved for the body of the form.
     */
    html_tree *formArea = container->new_node("div class=\"form\"");

    contact_table(formArea);
    build_call_table(formArea);
    build_faults_table(formArea);
    build_parts_table(formArea);
    build_labour_table(formArea);
    build_travel_table(formArea);

    // PAGE 2 (info page)
    page    = body->new_node("div class=\"page\"");
    subpage = page->new_node("div class=\"subpage\"");
    info_page(subpage);

    process_nodes(&dom);
    const char *html = dom.get_html();
    char       *buf  = strdup(html);
    return (buf);
}

/**
 * @brief main
 * @return 0
 *
 * This is a demo form using pretty_html to build a webpage and
 * then wkgtkprinter to turn it into a pdf.
 */
int main() {
    LOG_LEVEL  = LOG_DEBUG;
    LOG_IGNORE = iclog::LIB_HTML;

    // INITIALISE WEBKIT2GTK (SINGLETON)
    icGTK::init();

    const char *page = webpage();
    // MAKE A PDF FROM THE HTML
    PDFprinter  pdf;
    pdf.set_param(
        page,
        (std::filesystem::current_path().string() + "/demo-form.pdf").c_str()
    );
    pdf.layout("A4", "portrait");
    pdf.make_pdf();

    // WRITE OUT THE HTML
    std::ofstream file(std::filesystem::current_path().string() + "/demo-form.html");
    if (file) {
        file << page;
        file.close();
    }

    /**
     * @note html with an embeddded image is usually too large
     * to output to the journal.
     * */
    std::cout
        << "demo-form.html and demo-form.pdf have been generated"
        << std::endl;

    PDF_FreeHTML(page);

    return (0);
}
