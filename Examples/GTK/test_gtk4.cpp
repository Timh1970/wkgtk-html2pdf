#include <filesystem>
#include <gtk/gtk.h>
#include <iostream>
#include <webkit/webkit.h>
#include <wk2gtkpdf/ichtmltopdf++.h>
#include <wk2gtkpdf/pretty_html.h>

// Identical DOM logic - The Pimpl Promise kept
std::string generate_demo_html() {
    phtml::html_tree  dom("html");
    phtml::html_tree *head = dom.new_node("head");
    head->new_node("link rel=\"stylesheet\" href=\"/usr/share/wk2gtkpdf/A4-portrait.css\"");

    phtml::html_tree *body = dom.new_node("body");
    phtml::html_tree *page = body->new_node("div class=\"page\"");
    phtml::html_tree *sub  = page->new_node("div class=\"subpage\"");

    sub->new_node("h1")->set_node_content("GTK4 / WebKit6 Test");
    sub->new_node("p")->set_node_content("This is identical to the GTK3 version of this example, only built against gtk4.");

    phtml::process_nodes(&dom);
    return dom.get_html();
}

static void activate(GtkApplication *app, gpointer user_data) {
    std::string html = generate_demo_html();

    // 1. Modern GTK4 Window
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "wkgtk-html2pdf GTK4 Demo");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    // 2. WebKit6 View
    GtkWidget *webview = webkit_web_view_new();

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
    webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);

    webkit_web_view_load_html(
        WEBKIT_WEB_VIEW(webview),
        html.c_str(),
        "file:///"
    );

    gtk_window_set_child(GTK_WINDOW(window), webview);

    // 3. The Printer (Runs alongside the UI)
    phtml::PDFprinter pdf;
    std::string       out_path = std::filesystem::current_path().string() + "/demo_gtk4.pdf";
    pdf.set_param(html.c_str(), out_path.c_str());

    pdf.make_pdf();

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    // Using NON_UNIQUE to avoid D-Bus "Helper" headaches in testing
    GtkApplication *app = gtk_application_new("com.wkgtk.demo4", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
