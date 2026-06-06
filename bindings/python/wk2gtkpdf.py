import ctypes
from enum import IntEnum
import os

class WKGTKRunMode(IntEnum):
    KEEP_RUNNING = 0
    START_STOP = 1
    UNSET = 2

# Force standard multi-arch lookup or local directory fallback
def _load_library():
    possible_paths = [
        "libwk2gtkpdf-6.so.1",
        "libwk2gtkpdf-4.so.1",
        "./libwk2gtkpdf-6.so",
        "./libwk2gtkpdf-4.so",
        "/usr/lib/libwk2gtkpdf-6.so.1",
        "/usr/lib/libwk2gtkpdf-4.so.1"
    ]
    for path in possible_paths:
        try:
            return ctypes.CDLL(path)
        except OSError:
            continue
    raise ImportError("CRITICAL: libwk2gtkpdf runtime shared objects not found on this system.")

_lib = _load_library()

# Initialize the global GMainLoop background thread automatically on import
_lib.wk2gtk_init_engine.argtypes = [ctypes.c_int]
_lib.wk2gtk_init_engine.restype = None
_lib.wk2gtk_init_engine(int(WKGTKRunMode.KEEP_RUNNING))

# Map core factory methods
_lib.wk2gtk_printer_create.argtypes = []
_lib.wk2gtk_printer_create.restype = ctypes.c_void_p

_lib.wk2gtk_printer_set_param.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.wk2gtk_printer_set_param.restype = None

_lib.wk2gtk_printer_make_pdf.argtypes = [ctypes.c_void_p]
_lib.wk2gtk_printer_make_pdf.restype = None

_lib.wk2gtk_printer_destroy.argtypes = [ctypes.c_void_p]
_lib.wk2gtk_printer_destroy.restype = None

class PDFPrinter:
    def __init__(self):
        self.obj = _lib.wk2gtk_printer_create()
        if not self.obj:
            raise MemoryError("Failed to allocate native PDFprinter instance.")

    def __del__(self):
        if hasattr(self, 'obj') and self.obj:
            _lib.wk2gtk_printer_destroy(self.obj)

    def set_param(self, html: str, out_file: str):
        _lib.wk2gtk_printer_set_param(self.obj, html.encode('utf-8'), out_file.encode('utf-8'))

    def make_pdf(self):
        _lib.wk2gtk_printer_make_pdf(self.obj)

