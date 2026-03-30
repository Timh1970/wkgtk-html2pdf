# wkgtk-html2pdf

**Professional-grade, high-fidelity HTML-to-PDF typesetting engine.**

`wkgtk-html2pdf` is a modern, ABI-stable replacement for the unmaintained `wkhtmltopdf`. Built on **WebKitGTK** and **PoDoFo**, it treats HTML as a rigorous typesetting language. It is designed for industrial-scale PDF generation where layout precision, document validation, and server-side stability are non-negotiable.

## 🚀 Key Technical Advantages

### 1. Integer-Locked Layout (No "Creep")
Eliminates cumulative rounding drift. By utilizing a **0.25pt Quantized Grid** and synchronizing WebKit layout coordinates with physical PDF points, it ensures 1:1 parity between the browser rendering and the physical PDF grid.

### 2. Programmatic Form Generation (C++ DSL)
Includes a Pimpl-shielded **Fluent HTML Builder** that allows developers to construct complex, modular documents (like technical service reports or invoices) directly in C++. 
*   **Automated Hierarchy:** No manual string concatenation or tag closing required.
*   **Modular Design:** Easily build re-usable table components and data-driven forms.

### 3. Semantic Indexing & Bookmarking
WebKitGTK cannot natively produce internal PDF links. `wkgtk-html2pdf` solves this by injecting custom JavaScript to calculate coordinate-perfect anchor positions and using PoDoFo to stitch a semantic bookmark tree directly into the PDF.

### 4. Headless-First Infrastructure (Display Sentinel)
Features a **D-Bus aware Display Sentinel** that automatically provisions a hardened, systemd-managed `Xvfb` session if no display is detected. It actively suppresses desktop-environment bloat to ensure high-availability server stability.

### 5. Forensic Calibration Suite
Includes an integrated ANSI/ISO standardized calibration tester (`--calibrate`). This allows users to mathematically verify 1:1 parity between the browser rendering and the final PDF output.

---

## 🛠 Usage Example (Modular Form Building)

```cpp
void build_parts_table(html_tree *formArea) {
    auto table = formArea->new_node("table class=\"fixed\"");
    table->new_node("tr")->new_node("th colspan=\"3\"")->set_node_content("Parts");
    
    auto row = table->new_node("tr");
    row->new_node("td class=\"field\"")->set_node_content("1234567");
    row->new_node("td class=\"field\"")->set_node_content("Sample Part");
}
