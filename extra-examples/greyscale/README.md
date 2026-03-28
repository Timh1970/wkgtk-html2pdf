## Greyscale Example
One of the limitations of Webkit2GTK is that it is unable to make greyscale PDF's native.

As we did not anticipate there being a great deal of need to make greyscale PDF's we have not built functionality directly into the library however this is an example of how you can do it if you wish.

### Building
In order to build this example you will need to install ImageMagick

Arch
>pacman -Syy imagemagick 
	
Debian
>apt install libmagick++-dev 

If not already installed; then just make and run 
> ./greyscale
to generate 
	
- An A5 Colour html page
- An A5 Colour PDF
- An A5 Greyscale PDF

**Note:** In this example rather than writing the file out we use a PDFprinter::blob which is compatible with ImageMagick's Magick::Blob.  This saves you having to handle temporary files when post processing.