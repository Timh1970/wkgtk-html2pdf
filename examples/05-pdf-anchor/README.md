## Index Test

This is a basic hello world style example for generating links.

To run this example, copy this directory to your home folder:

> cp -r /usr/share/doc/libwk2gtkpdf-dev/examples/05-pdf-anchor ~/

Then run make and 

> ./indextest

`./indextest` will generate 3 files 

>	indexed.pdf
>
>	noindex.pdf
 
And;

> testpage_anchor_only.html

`indexed.pdf` will be a clickable pdf file while `noindex.pdf` will not. `testpage_anchor_only.html` is the raw HTML from which the files were generated.

To remove the application and the generated files run   

> make clean

**NOTE**:  For more complete indexing tests with bookmarks see the **06-html-tests** folder; or for the ultimate indexing and bookmarking example take a look at the code behind our manual at https://wkgtk-html2pdf.com