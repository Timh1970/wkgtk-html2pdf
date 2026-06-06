#include "c_bridge.h"

#include "ichtmltopdf++.h"

#ifdef __cplusplus
extern "C" {
#endif

void wk2gtk_init_engine(int run_mode_val) {
    WKGTKRunMode mode = WKGTKRunMode::KEEP_RUNNING;
    if (run_mode_val == 1)
        mode = WKGTKRunMode::START_STOP;
    if (run_mode_val == 2)
        mode = WKGTKRunMode::UNSET;

    // Triggers your native Meyers Singleton securely
    icGTK::init(mode);
}

void *wk2gtk_printer_create() {
    // Return as an opaque void pointer to Python/C layers
    return reinterpret_cast<void *>(new phtml::PDFprinter());
}

void wk2gtk_printer_set_param(void *printer, const char *html, const char *out_file) {
    if (printer && html && out_file) {
        phtml::PDFprinter *real_printer = reinterpret_cast<phtml::PDFprinter *>(printer);
        real_printer->set_param(html, out_file);
    }
}

void wk2gtk_printer_make_pdf(void *printer) {
    if (printer) {
        phtml::PDFprinter *real_printer = reinterpret_cast<phtml::PDFprinter *>(printer);
        real_printer->make_pdf();
    }
}

void wk2gtk_printer_destroy(void *printer) {
    if (printer) {
        phtml::PDFprinter *real_printer = reinterpret_cast<phtml::PDFprinter *>(printer);
        delete real_printer;
    }
}

#ifdef __cplusplus
}
#endif
