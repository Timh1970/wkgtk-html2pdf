#!/usr/bin/env python3
import os
import sys

# Append the system-wide installation path or local testing path to the search path
sys.path.append('/usr/lib/python3/dist-packages')
sys.path.append(os.path.abspath('../../bindings/python'))

from wk2gtkpdf import PDFPrinter

print("[Python Example] Activating concurrent WebKitGTK engine...")
printer = PDFPrinter()

html_content = "<html><body><h1>Python Print Output</h1><p>Stable, headless PDF generation.</p></body></html>"
output_path = os.path.abspath("./python_print_demo.pdf")

printer.set_param(html_content, output_path)
printer.make_pdf()

print(f"[Python Example] Production PDF generated successfully at: {output_path}")

