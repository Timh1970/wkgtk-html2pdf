## Status
We are currently preparing the package to submit to the Debian archive as of writing we have sanitised it and it now passes all the lintian checks and is split into 3 packages as Debian likes it:

We are in the process of setting up a web site, and once that is up and running (hopefully in the next few days) we will begin the submission process.  In the meantime if you wish to install it then you can download the packages as of your requirement.  

In all cases you will need the library.  If you wish to utilise the api *-dev* package and if you want to use the cli you will need the wkgtk-html2pdf.  If you are unsure then just download all 3 and install with `dpkg -i *.deb` (recommended).

- **library** - https://git.inplico.uk/releases/wkgtk-html2pdf/Deb/0.0.22/libwk2gtkpdf0_0.0.22-1_amd64.deb
- **CLI** - https://git.inplico.uk/releases/wkgtk-html2pdf/Deb/0.0.22/wkgtk-html2pdf_0.0.22-1_amd64.deb
- **C++ API** - https://git.inplico.uk/releases/wkgtk-html2pdf/Deb/0.0.22/libwk2gtkpdf0-dev_0.0.22-1_amd64.deb
