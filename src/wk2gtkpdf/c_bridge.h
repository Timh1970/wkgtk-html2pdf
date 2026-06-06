#ifndef WK2GTKPDF_C_BRIDGE_H
#define WK2GTKPDF_C_BRIDGE_H

#ifndef PDF_API
#define PDF_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Universal, version-agnostic C signatures for external runtimes
PDF_API void  wk2gtk_init_engine(int run_mode_val);
PDF_API void *wk2gtk_printer_create();
PDF_API void  wk2gtk_printer_set_param(void *printer, const char *html, const char *out_file);
PDF_API void  wk2gtk_printer_make_pdf(void *printer);
PDF_API void  wk2gtk_printer_destroy(void *printer);

#ifdef __cplusplus
}
#endif

#endif // WK2GTKPDF_C_BRIDGE_H
