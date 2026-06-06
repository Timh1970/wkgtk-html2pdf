package main

/*
#cgo LDFLAGS: -L/usr/lib -L. -lwk2gtkpdf-6
#include <stdlib.h>

// Directly declare your flat C function signatures right inside the comment block!
void wk2gtk_init_engine(int run_mode_val);
void* wk2gtk_printer_create();
void wk2gtk_printer_set_param(void* printer, const char* html, const char* out_file);
void wk2gtk_printer_make_pdf(void* printer);
void wk2gtk_printer_destroy(void* printer);
*/
import "C"
import (
	"fmt"
	"unsafe"
)

func main() {
	fmt.Println("[Go Engine] Activating background GLib loop thread...")
	
	// 1. Automatically initialize your Meyers Singleton loop daemon
	C.wk2gtk_init_engine(0) // 0 = WKGTKRunMode::KEEP_RUNNING

	// 2. Instantiate your memory-isolated C++ printer instance on the heap
	printer := C.wk2gtk_printer_create()
	defer C.wk2gtk_printer_destroy(printer) // Guarantees zero memory leaks on exit

	htmlContent := "<html><body><h1>Go Microservice Output</h1><p>High-concurrency PDF rendering.</p></body></html>"
	outputPath := "./go_print_demo.pdf"

	// 3. Convert Go strings to C-compatible char arrays safely
	cHtml := C.CString(htmlContent)
	cOut := C.CString(outputPath)
	defer C.free(unsafe.Pointer(cHtml))
	defer C.free(unsafe.Pointer(cOut))

	// 4. Pass execution straight down to your C++ core via cgo memory addresses
	C.wk2gtk_printer_set_param(printer, cHtml, cOut)
	
	fmt.Println("[Go Engine] Dropping out of Go runtime to execute WebKit print thread...")
	C.wk2gtk_printer_make_pdf(printer)

	fmt.Println("[Go Engine] Success! PDF compiled cleanly via native WebKitGTK core.")
}

