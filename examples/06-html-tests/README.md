## HTML Tests
In this folder you will find a selection of HTML files that are used to test various functionality of the library via the command line interface

### overflowing_index_test.html
This file tests indexing when the link overflows from one line onto the next.

```
wkgtk-html2pdf -i overflowing_index_test.html -o overflowing_index_test.pdf --index enhanced
```
The generated PDF has 2 links that overflow onto the next line, the test demonstrates that both links are clickable and navigate to the desired page no matter where you click on them.

This also demonstrates how to create a bookmark bar.


### test_modes.html

Tests the indexing by creating a clean properly nested PDF index even when the links are not in any particular order.

```
wkgtk-html2pdf -i test_modes.html -o test_modes.pdf --index enhanced   
```

### Other tests
The biggest test of the link system is the user manual at https://wkgtk-html2pdf.com  There you will see the sidebar links however this is pure html and Javascript.  You can download the raw manual at https://wkgtk-html2pdf.com/doc-repo/api-manual.html and run   
  
```
wkgtk-html2pdf -i api-manual.html -o api-manual.pdf --index enhanced 
```
and you will have a fully indexed pdf version of the manual formatted identical to the screen.

### Calibrator
`wkgtk-html2pdf` includes a built-in calibration engine that generates a 150-page high-precision test grid. This is used to verify that your rendering engine is correctly synchronising the Typographic Grid (Points) with the Hardware Raster (Pixels).

#### ISO (A4)

```
wkgtk-html2pdf --calibrate ISO
```

#### ANSI A (US Letter)

```
wkgtk-html2pdf --calibrate US
```

How to use the calibration test and why it exists is written into the technical manual.