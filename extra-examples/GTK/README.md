

  

The Architecture: "The Pimpl Chameleon"
Unlike traditional libraries that force you to choose a single toolkit, wkgtk-html2pdf uses the Pimpl (Pointer to Implementation) idiom and Symbol Versioning to offer dual-engine support.

The examples in this folder demonstrate how you can utilise wkgtk-html2pdf in both GTK3 and GTK4 applications simultaneously. The benefits are:

 - Stable C++ API: Write your code once; it works identically for GTK3 or GTK4.
 - ABI Isolation: Internal WebKit/GTK symbols are hidden. No "Dependency Leakage" into your application.
 - Co-installable: You can have both the GTK3 and GTK4 engines installed side-by-side without conflicts.

Installation (Debian Trixie/Sid)

As of writing we are currently hoping to get sponsorship to put wkgtk_html2pdf in the Debian archive; Should we achieve that goal the simplest way to get the full suite (CLI and both engines) will be:

```
sudo apt install wkgtk-html2pdf
```

This will install the command line interface along with both versions of the library. In the meantime, while we are making our way through the application process you can download the individual deb files here:

> https://git.inplico.uk/releases/wkgtk-html2pdf/Deb/

**NOTE**: We advise against dowloading releases earlier than version 1 as they are considered volatile and lack features and support for GTK4. 



## GTK3:
Link against the versioned -4 engine to build the test example for GTK3 as follows.
```
g++ test_gtk4.cpp -o test_gtk4 $(pkg-config --cflags --libs gtk3 webkitgtk-4.0 wk2gtkpdf-4)
```

When you run the application it will bring up a window with a simple hello world example and produce a PDF in your current folder
## GTK4
To try the GTK4 example the process is identical save for the versions passed to pkg-config


```
g++ test_gtk4.cpp -o test_gtk4 $(pkg-config --cflags --libs gtk4 webkitgtk-6.0 wk2gtkpdf-6) -std=c++20 
```

**NOTE** The margin that appears on the viewer is not visible on the pdf.
