## Demo Form
This is a more complex example that generates a form utilising a series of tables within a grid.

**NOTE**: Hidden within this example's CSS is a snippet that resovles a well known problem when using `<br>` elements.

This form's CSS template includes a targeted fix for the common "compounded spacing" issue associated with `<br>` elements. Standard browser rendering of line breaks can subtly distort the vertical rhythm of explicit tables. Our standard forces `<br>` to behave as a pure layout break (display: block) with zero padding or margin, ensuring 1:1 parity between your CSS definitions and the final PDF raster.

```
br {
    display: block;
    content: "";
    margin: 0;
    padding: 0;
}
```

To run this example, copy this directory to your home folder:

> cp -r /usr/share/doc/libwk2gtkpdf-dev/examples/04-business-form ~/

Then run make and 

> ./demoform

`./demoform` will generate 2 files 

> demo-form.pdf
 
And;

> demo-form.html

To remove the application and the generated files run   

> make clean