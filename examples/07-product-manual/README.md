## wkgtk-html2pdf documenation
This is us eating our own dog food.  It is the HTML version of the manual that will allow you to generate a real life PDF manual

To run this example, copy this directory to your home folder:

> cp -r /usr/share/doc/libwk2gtkpdf-dev/examples/07-product-manual ~/

and then from that folder run 

```
wkgtk-html2pdf -i api-manual.html -o manual.pdf -O portrait -r --size A4 -v7 --index enhanced
```
and you will have a pdf version of the manual that looks identical to the html version called `manual.pdf`

**NOTE**: This version of the manual is not regularly updated as it is considered an example.  For the most up to date documentation visit https://wkgtk-html2pdf.com

If you open the manual.pdf file in a viewer like **Okular** and scale it to &asymp; 88.2% and compare any page to the HTML page in the browser at 100% they should be indistinguishable to the human eye.