#include <filesystem>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/pretty_html.h>

std::string generate_demo_html() {
    phtml::html_tree  dom("html");
    phtml::html_tree *head = dom.new_node("head");
    // Link to the system-installed CSS templates
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait.css\"");

    phtml::html_tree *body = dom.new_node("body");
    phtml::html_tree *page = body->new_node("div class=\"page\"");
    phtml::html_tree *sub  = page->new_node("div class=\"subpage\"");

    sub->new_node("h1")->set_node_content("GTK Viewer Demo");
    sub->new_node("p")->set_node_content(
        "This example tests integration into GTK3 applications withouth "
        "the need to utilise the built back end <b>icGTK<b/>"
    );

    phtml::process_nodes(&dom);
    return dom.get_html();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    std::string html = generate_demo_html();

    // 1. The UI Part (The Viewer)
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *webview = webkit_web_view_new();

    webkit_web_view_load_html(
        WEBKIT_WEB_VIEW(webview),
        html.c_str(),
        "file:///"
    );

    gtk_container_add(GTK_CONTAINER(window), webview);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);

    // 2. The Library Part (The Printer)
    // Even while the window is open, your Pimpl engine does its work
    phtml::PDFprinter pdf;
    std::string       out_path = std::filesystem::current_path().string() + "demo_gtk3.pdf";
    pdf.set_param(html.c_str(), out_path.c_str());
    pdf.make_pdf();

    gtk_main();
    return 0;
}
