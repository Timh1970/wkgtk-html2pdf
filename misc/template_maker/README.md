## Template Makker

This is a utility app to generate the stylesheets for each suported page size.  it can be compiled with a simple

```
g++ -o template_maker template_maker.cpp
```

This now has the ability to set page margins to the templates in bulk using the `-m` flag.  It will default to 6mm if no value is passed.

The value is numeric only and in millimetres (mm).

eg.

```bash
template_maker -m 8
```
will generate the templates with an 8mm page margin.

**NOTE:** If you run into problems with page margins being wider than expected then check the application that you are printing from.  In tests Okular added its own margin and scaled the pages accordingly whereas when printing the pdf from Chrome the margins were respected.