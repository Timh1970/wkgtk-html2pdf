<?php

class WK2GTKPrinter {
    private $ffi;
    private $printer;

    public function __construct(?string $base_uri = null) {
        $this->ffi = FFI::cdef(
            file_get_contents("/usr/include/wk2gtkpdf/c_bridge.h"), 
            "libwk2gtkpdf-6.so"
        );

        $this->printer = $this->ffi->wk2gtk_printer_create();
    }

    public function __destruct() {
        if ($this->printer) {
            $this->ffi->wk2gtk_printer_destroy($this->printer);
        }
    }

    public function setParam(string $html, string $outFile): void {
        $this->ffi->wk2gtk_printer_set_param($this->printer, $html, $outFile);
    }

    public function makePdf(): void {
        $this->ffi->wk2gtk_printer_make_pdf($this->printer);
    }
}

