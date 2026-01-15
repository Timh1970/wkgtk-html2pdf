#include <filesystem>
#include <fstream>
#include <iostream>
#include <wk2gtkpdf/encode_image.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/iclog.h>
#include <wk2gtkpdf/pretty_html.h>

void contact_table(html_tree *subpage) {

    html_tree *table = subpage->new_node("table class=\"fixed\"");
    table->new_node("col width=\"14%\"");
    table->new_node("col width=\"43%\"");
    table->new_node("col width=\"43%\"");

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

    html_tree *cAddress = row->new_node("td class=\"field\"");
    cAddress->new_node("br")->set_node_content("31 Spooner Street");
    cAddress->new_node("br")->set_node_content("Quahog");
    cAddress->new_node("br")->set_node_content("Cranston");
    cAddress->new_node("br")->set_node_content("Rhode Island");

    // SITE ADDRESS
    html_tree *sAddress = row->new_node("td class=\"field\"");
    sAddress->new_node("br")->set_node_content("1407 Greymalkin Lane");
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

    // NOTES
    row = table->new_node("tr");
    row->new_node("td class=\"label\" style=\"border-top: none;\"")
        ->set_node_content("Notes:");
    row->new_node("td class=\"faultfield\" colspan=\"2\"")
        ->set_node_content("Additional notes go here");
}

void build_call_table(html_tree *subpage) {

    html_tree *table = subpage->new_node("table class=\"fixed\"");
    table->new_node("col width=\"14%\"");
    table->new_node("col width=\"20%\"");
    table->new_node("col width=\"11%\"");
    table->new_node("col width=\"55%\"");

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

void build_faults_table(html_tree &subpage) {
    // FAULT TABLE HEADERS
    html_tree *table = subpage.new_node("table class=fixed");
    table->new_node("col width=14%");
    table->new_node("col width=67%");
    table->new_node("col width=19%");
    table->new_node("tr")
        ->new_node("th colspan=\"3\"")
        ->set_node_content("Faults");

    html_tree *row = table->new_node("tr");
    row->new_node(
        "td class=label style=\"border-right: none; border-bottom: "
        "none;\" colspan=\"2\""
    );
    row->new_node("td class=label style=\"border-left: none;\"")
        ->set_node_content("Status:");

    for (int i = 0; i != 4; ++i) {
        row = table->new_node("tr");
        // DESCRIPTION LABEL
        row->new_node(
               "td class=label style=\"border-top: none; border-bottom: none;\""
        )
            ->set_node_content("Description:");
        // DESCRIPTION FIELD
        row->new_node("td class=faultfield");
        // STATUS FIELD
        row->new_node("td class=faultfield rowspan=\"2\"");

        row = table->new_node("tr");
        // NOTES LABEL
        row->new_node("td class=label style=\"border-top: none;\"")
            ->set_node_content("Notes:");
        row->new_node("td class=faultfield");
    }
}

void build_parts_table(html_tree *subpage) {
    html_tree *table = subpage->new_node("table class=\"fixed\"");
    table->new_node("col width=\"10%\"");
    table->new_node("col width=\"20%\"");
    table->new_node("col width=\"70%\"");
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

void build_labour_table(html_tree *subpage) {

    html_tree *table = subpage->new_node("table class=\"fixed\"");
    table->new_node("col width=\"20%\"");
    table->new_node("col width=\"32.5%\"");
    table->new_node("col width=\"32.5%\"");
    table->new_node("col width=\"15%\"");
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

void build_travel_table(html_tree *subpage) {

    html_tree *table = subpage->new_node("table class=\"fixed\"");
    table->new_node("col width=\"46%\"");
    table->new_node("col width=\"37%\"");
    table->new_node("col width=\"7%\"");
    table->new_node("col width=\"10%\"");

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
           "td class=\"ssaverlabel\" style=\"border-left: none; "
           "border-right: none;\""
    )
        ->set_node_content("Miles:");
    row->new_node("td class=\"ssaverlabel\" style=\"border-left: none;\"")
        ->set_node_content("Time:");

    for (int i = 0; i != 4; ++i) {

        row = table->new_node("tr");
        row->new_node("td class=\"field\"");
        row->new_node("td class=\"field\"");
        row->new_node("td class=\"ssaverfield\" style=\"text-align:right\"");
        row->new_node("td class=\"ssaverfield\"");
    }

    // TOTALS
    row = table->new_node("tr");
    row->new_node("td class=\"label\" colspan=2 style=\"text-align: right;\"")
        ->set_node_content("Totals:");
    row->new_node("td class=\"ssaverfield\" style=\"text-align:right\"");
    row->new_node("td class=\"ssaverfield\"");
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
        "It can be compiled using the make in this folder.  When run it will "
        "automatically generate this page in html and the pdf of the same name "
        "in this folder."
    );

    subpage->new_node("h2")->set_node_content("Margins");
    subpage->new_node("p")->set_node_content(
        "Margins are set in 3 different places and each can conflict.  The "
        "margin on this page is set in the stylesheet(in this instance "
        "<i>/usr/share/wkgtkprinter/a4-portrait-pdf.css</i>).  It can be "
        "changed by setting the variable \"<b>--page-margin:</b>\" towards "
        "the top of the stylesheet.When the stylesheet is referenced you "
        "will see a red box indicating the margin to aid in designing the "
        "form (this box does not appear in the PDF)."
    );

    subpage->new_node("p")->set_node_content(
        "Page margins can also be set in the page configuration file "
        "\"<i>/usr/share/wkgtkprinter/a4-portrait-pdf.page</i>\".They default "
        "to 0 as margins are handeled in the CSS."
    );

    subpage->new_node("p")->set_node_content(
        "Some applications also set their own page margins when printing. "
        "During testing these have been accumulative so if you set a 6mm "
        "margin in the CSS and the application's print dialog has a 4mm "
        "margin the resultant page will have a 10mm margin."
    );

    subpage->new_node("h2")->set_node_content("Style Sheets");

    subpage->new_node("p")->set_node_content(
        "Each document should use a base stylesheet that is directly linked. "
        "Any custom base stylesheets should be stored in \"/usr/share/wkgtkprinter\" "
        "and should be linked using the absolute path.The base style sheet provides "
        "you with a template to develop your document."
    );

    subpage->new_node("p")->set_node_content(
        "For pdf development it is better to embed styles rather than have separate "
        "css files however wkgtkprinter++' pretty_html can handle this internally if "
        "you read an external stylesheet into a &lt;style&gt; element. If you are "
        "gong to link them then it is recommended that you place them in a location "
        "accessible by wkgtkprinter such as \"</i>/usr/share/wkgtkprinter/</i>\"."
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

    subpage->new_node("h2")->set_node_content("Images");
    subpage->new_node("p")->set_node_content(
        "For the purposes of generating PDF's images are embeded within the HTML.  "
        "They are base64 encoded which does make them a little larger, but the design "
        "decision was taken to prefer reliability over file size."
    );
    subpage->new_node("p")->set_node_content(
        "wkgtkprinter++ has a built in class for encoding images called "
        "<i>encode_image</i> (see example)."
    );
}

std::string webpage() {

    jlog << iclog::loglevel::debug << iclog::category::CLI
         << "Constructing html form."
         << std::endl;

    std::string html("<!DOCTYPE html>");

    // PRIMARY ELEMENT
    html_tree dom("html", html);

    // HEAD
    html_tree *head = dom.new_node("head");
    head->new_node("meta charset=\"UTF-8\"");

    // STYLE
    /**
     * Built in style for an ISO A4 page (see templates folder)
     * */
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait.css\"");

    /**
     * @brief f
     *
     * Embed the style here; it doesn't need to be embedded if
     * it is in an accessible location.
     */
    std::ifstream f("demo-form.css");
    std::string   style((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    head->new_node("style")->set_node_content(style);

    // BODY
    html_tree *body    = dom.new_node("body");
    // PAGE
    html_tree *page    = body->new_node("div class=\"page\"");
    html_tree *subpage = page->new_node("div class=\"subpage\"");

    // HEADING
    html_tree *container = subpage->new_node("div style=\"position: relative\"");

    // LOGO
    encode_image img("example-logo.png");
    container->new_node("img class=\"logo\" src=" + img.b64_image());

    // TITLE
    container->new_node("h1 class=\"doctitle\"")->set_node_content("DEMO Job sheet");

    // REFERENCE
    container->new_node("h4 class=\"ref\"")
        ->set_node_content("Ref: 1234/<br>20260111");

    contact_table(subpage);
    build_call_table(subpage);
    build_faults_table(*subpage);
    build_parts_table(subpage);
    build_labour_table(subpage);
    build_travel_table(subpage);

    // PAGE 2 (info page)
    page    = body->new_node("div class=\"page\"");
    subpage = page->new_node("div class=\"subpage\"");
    info_page(subpage);

    pretty_html::process_nodes(&dom);

    return (html);
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

    WEBPAGE html = webpage();

    // MAKE A PDF FROM THE HTML
    PDFprinter pdf;
    pdf.set_param(
        html,
        PDFprinter::read_file("/usr/share/wkgtkprinter/a4-portrait-pdf.page"),
        std::filesystem::current_path().string() + "/demo-form.pdf"
    );
    pdf.make_pdf();

    // WRITE OUT THE HTML
    std::ofstream file(std::filesystem::current_path().string() + "/demo-from.html");
    if (file) {
        file << html;
        file.close();
    }

    /**
     * @note html with an embeddded image is usually too large
     * to output to the journal.
     * */
    std::cout
        << "demo-form.html and demo-form.pdf have been generated"
        << std::endl;

    return (0);
}
