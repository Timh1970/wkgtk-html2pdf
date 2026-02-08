## Indexing Example - EXPERIMENTAL
We have not found a way to get webkit2gtk to reliably handle index links and anchors internally and therefore do some post processing.  This involves grabbing the coordinates of the indexes and the anchors using some injected javascript; building a list of the coordinates and then passing that list to podofo to post process.

>**index-test.html** (for information purposes) -  This file contains the test HTML along with the javascript that was used to obtain the coordinates.

>**testpage.html** - This is a sanitised version of index-test.html with the javascript removed used for testing the functionality  

>**noindex.pdf** - A generated version of the pdf without links.

>**indexed.pdf** - The final version with an index.

**NOTE:** For testing purposes we have placed a bounding box on the HTML and another one of the rectangle that overlays it.  This this second overlay box will not be visible if you recompile and run the **./indextest** application. 

- Indexing is not enabled by default, you need to explicitly enable it.
- Please report any bugs or issues as this has not been exhaustively tested.

This has introduced a dependency of PoDoFo which has different versions across different versions of linux.  In particular Debian uses 0.9 while Arch uses 0.10.

Currently this is to be considered **experimental**


## Modes
### Classic mode:
In classic mode wkgtk-html2pdf parses traditional anchors `<a>` and they work in the exact same manner as they would in html.

**NOTE**: External links are not supported 

### Enhanced
In enhanced mode you need to wrap your anchors and anything else in a `<div class="index-item">` element.  This is useful if you wish to extend the clickable area beyond the `<a>` element.


To utilise the built in indexing the source element must have a class name of "**index_item**" eg.

```
div class="index-item"><a href="#anchor1">Anchor 1</a></div>
```

**NOTE:** 
- The modes should not be mixed.  If used in ***Classic*** mode all '<a href="#...' elements will be processed; `<div class="index-item">` elements will be ignored.
- When used in enhanced mode any `<a>` elements that are not wrapped in a `<div class="index-item">` will be ignored.


### Mandatory elements

It is mandatory to use the `<div class="page">` element as this is needed by the javascript that gathers the coordinates relative to the page

Have a look at testpage.html to see how to format compatible HTML.

**NOTE:** It is not currently possible to get a BLOB if you are indexing the pdf; for now a workaround is to write to a temporary location if you wish to conduct any post processing.


