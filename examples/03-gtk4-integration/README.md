## Test GTK4
This is a simple app to demonstrate integration with GTK3 applications.

To run this example, copy this directory to your home folder:

- cp -r /usr/share/doc/libwk2gtkpdf-dev/examples/03-gtk4-integration ~/

Then run make and 

- ./testgtk3.

`./testgtk3` will display an HTML file in a GTK window and generate that html into a PDF called `demo_gtk4.pdf` in the current working folder.

To remove it and the artefacts it generates run 

- make clean


**NOTE**: For GTK4 applications you need to build against the correct GTK version of the library.

> `pkg-config --libs --cflags wk2gtkpdf-6`