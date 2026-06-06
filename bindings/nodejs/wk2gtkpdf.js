const ffi = require('ffi-napi');
const path = require('path');

// Identify and load the underlying shared object library binary safely
function loadLibrary() {
    const possiblePaths = [
        'libwk2gtkpdf-6.so.1',
        'libwk2gtkpdf-4.so.1',
        './libwk2gtkpdf-6.so',
        './libwk2gtkpdf-4.so',
        '/usr/lib/libwk2gtkpdf-6.so.1',
        '/usr/lib/libwk2gtkpdf-4.so.1'
    ];
    for (const libPath of possiblePaths) {
        try {
            // Define your exact C-bridge function signature arguments and return structures
            return ffi.Library(libPath, {
                'wk2gtk_init_engine': ['void', ['int']],
                'wk2gtk_printer_create': ['pointer', []],
                'wk2gtk_printer_set_param': ['void', ['pointer', 'string', 'string']],
                'wk2gtk_printer_make_pdf': ['void', ['pointer']],
                'wk2gtk_printer_destroy': ['void', ['pointer']]
            });
        } catch (e) {
            continue;
        }
    }
    throw new Error("CRITICAL: libwk2gtkpdf native engine shared object files not found on this system.");
}

const lib = loadLibrary();

// Automatically trigger your C++ Meyers Singleton loop daemon immediately on module load!
// This starts your std::thread background runtime cleanly inside the Node process space
lib.wk2gtk_init_engine(0); // 0 = WKGTKRunMode::KEEP_RUNNING

class PDFPrinter {
    constructor() {
        this.obj = lib.wk2gtk_printer_create();
        if (this.obj.isNull()) {
            throw new Error("Failed to allocate native PDFprinter heap memory.");
        }
        
        // Ensure JavaScript garbage collection automatically fires your C++ destructor!
        this._finalizer = registry.register(this, this.obj, this.obj);
    }

    setParam(html, outFile) {
        lib.wk2gtk_printer_set_param(this.obj, html, outFile);
    }

    makePdf() {
        // Drops out of JavaScript entirely, freeing the V8 event loop 
        // while your C++ engine renders the PDF on your background thread
        lib.wk2gtk_printer_make_pdf(this.obj);
    }
}

// Memory clean-up registry hook to stop V8 engine leaks permanently
const registry = new FinalizationRegistry((heldValue) => {
    if (heldValue) {
        lib.wk2gtk_printer_destroy(heldValue);
    }
});

module.exports = { PDFPrinter };

