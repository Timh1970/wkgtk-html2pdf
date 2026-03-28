TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= link_pkgconfig

INCLUDEPATH += /usr/include/ImageMagick-7
INCLUDEPATH += /usr/include/podofo-0.9
INCLUDEPATH += /usr/include/libxml2
INCLUDEPATH += /usr/include/glib-2.0
INCLUDEPATH += /usr/include/gtk-3.0
INCLUDEPATH += /usr/include/pango-1.0
INCLUDEPATH += /usr/lib/glib-2.0/include
INCLUDEPATH += /usr/include/harfbuzz
INCLUDEPATH += /usr/include/cairo
INCLUDEPATH += /usr/include/gdk-pixbuf-2.0
INCLUDEPATH += /usr/include/atk-1.0
INCLUDEPATH += /usr/include/libsoup-3.0
INCLUDEPATH += /usr/include/webkitgtk-4.1

#--- WEB KIT 6.0 / GTK4 ADAPTATION ---
# INCLUDEPATH += \
#     /usr/include/webkitgtk-6.0 \
#     /usr/include/gtk-4.0 \
#     /usr/include/glib-2.0 \
#     /usr/lib/glib-2.0/include \
#     /usr/include/pango-1.0 \
#     /usr/include/harfbuzz \
#     /usr/include/cairo \
#     /usr/include/gdk-pixbuf-2.0 \
#     /usr/include/libsoup-3.0 \
#     /usr/include/graphene-1.0 \
#     /usr/lib/graphene-1.0/include \
#     /usr/include/podofo-0.9 \
#     /usr/include/ImageMagick-7

# --- WPE 2 (webkit embedded) Stack ---
# INCLUDEPATH += /usr/include/cairo
# INCLUDEPATH += /usr/include/glib-2.0
# INCLUDEPATH += /usr/lib/glib-2.0/include
# INCLUDEPATH += /usr/include/gio-unix-2.0
# INCLUDEPATH += /usr/include/pixman-1
# INCLUDEPATH += /usr/include/freetype2
# INCLUDEPATH += /usr/include/libpng16
# INCLUDEPATH += /usr/include/libsoup-3.0
# INCLUDEPATH += /usr/include/wpe-1.0
# INCLUDEPATH += /usr/include/wpe-fdo-1.0
# INCLUDEPATH += /usr/include/wpe-webkit-2.0
# INCLUDEPATH += /usr/include/wpe-webkit-2.0/wpe-platform

PKGCONFIG += wpe-webkit-2.0 cairo-pdf

SOURCES += \
        Examples/GTK/test_gtk3.cpp \
        Examples/GTK/test_gtk4.cpp \
        Examples/GTK3/viewer_gtk3.cpp \
        Examples/GTK4/test_gtk4.cpp \
        Examples/demo-form/demo_jobsheet.cpp \
        Examples/demo_jobsheet.cpp \
        Examples/greyscale/greyscale.cpp \
        Examples/hello-page/hello_pdf.cpp \
        Examples/html-tests/gridtest.cpp \
        Examples/indexing-tests/indextest.cpp \
        examples/01-minimal-cpp/hello_pdf.cpp \
        examples/02-gtk3-integration/test_gtk3.cpp \
        examples/03-gtk4-integration/test_gtk4.cpp \
        examples/04-business-form/demo_jobsheet.cpp \
        examples/05-pdf-bookmark/indextest.cpp \
        examples/demo-form/demo_jobsheet.cpp \
        extra-examples/greyscale/greyscale.cpp \
        extra-examples/html-tests/gridtest.cpp \
        extra-examples/indexing-tests/indextest.cpp \
        misc/template_maker/template_maker.cpp \
        src/cli++/main.cpp \
        src/cli/main.cpp \
        src/log++/ic_printerlog++.cpp \
        src/wk2gtkpdf/cairo_painter.cpp \
        src/wk2gtkpdf/encode_image.cpp \
        src/wk2gtkpdf/ichtmltopdf++.cpp \
        src/wk2gtkpdf/ichtmltopdf_int.cpp \
        src/wk2gtkpdf/iclog.cpp \
        src/wk2gtkpdf/index_pdf.cpp \
        src/wk2gtkpdf/pretty_html.cpp

DISTFILES += \
        Examples/GTK/README.md \
        Examples/demo-form/demo-form.css \
        Examples/demo-form/demo-form.pdf \
        Examples/demo-form/example-logo.png \
        examples/01-minimal-cpp/README.md \
        examples/02-gtk3-integration/testgtk3 \
        examples/02-gtk3-integrationdemo_gtk3.pdf \
        examples/demo-form/README.md \
        examples/demo-form/cli-demo-form.pdf \
        examples/demo-form/demo-form.css \
        examples/demo-form/demo-form.pdf \
        examples/demo-form/demo-from-tmplate.html \
        examples/demo-form/demo-from.html \
        examples/demo-form/demoform \
        examples/demo-form/example-logo.png \
        extra-examples/Calibration tests/wkgtk-html2pdf-cal-2026-03-25_18-03-09.html \
        extra-examples/Calibration tests/wkgtk-html2pdf-cal-2026-03-25_18-03-09.pdf \
        extra-examples/Calibration tests/wkgtk-html2pdf-cal-2026-03-25_18-21-31.html \
        extra-examples/Calibration tests/wkgtk-html2pdf-cal-2026-03-25_18-21-31.pdf \
        extra-examples/GTK/README.md \
        extra-examples/GTK/demo_gtk4.pdf \
        extra-examples/GTK/demo_output.pdf \
        extra-examples/README.md \
        extra-examples/demo-form/README.md \
        extra-examples/demo-form/cli-demo-form.pdf \
        extra-examples/demo-form/demo-form.css \
        extra-examples/demo-form/demo-form.pdf \
        extra-examples/demo-form/demo-from-tmplate.html \
        extra-examples/demo-form/demo-from.html \
        extra-examples/demo-form/demoform \
        extra-examples/demo-form/example-logo.png \
        extra-examples/greyscale/A2.pdf \
        extra-examples/greyscale/README.md \
        extra-examples/greyscale/example-logo.png \
        extra-examples/greyscale/output_color.html \
        extra-examples/greyscale/output_color.pdf \
        extra-examples/greyscale/output_gs.pdf \
        extra-examples/html-tests/A0-portrait.css \
        extra-examples/html-tests/A0P-measure_test.html \
        extra-examples/html-tests/A0P-measure_test.pdf \
        extra-examples/html-tests/A3-jobsheet.html \
        extra-examples/html-tests/A3-jobsheet.pdf \
        extra-examples/html-tests/A3-portrait.css \
        extra-examples/html-tests/A3P-measure_test.html \
        extra-examples/html-tests/A3P-measure_test.pdf \
        extra-examples/html-tests/A4-1000-measure_test.html \
        extra-examples/html-tests/A4-1000-measure_test.pdf \
        extra-examples/html-tests/A4-jobsheet.html \
        extra-examples/html-tests/A4-jobsheet.pdf \
        extra-examples/html-tests/A4-portrait.css \
        extra-examples/html-tests/A4P-measure_test.html \
        extra-examples/html-tests/A4P-measure_test.pdf \
        extra-examples/html-tests/A5-landscape.css \
        extra-examples/html-tests/A5-portrait.css \
        extra-examples/html-tests/A5L-measure_test.html \
        extra-examples/html-tests/A5L-measure_test.pdf \
        extra-examples/html-tests/A6-500page-ls.pdf \
        extra-examples/html-tests/A6-landscape.css \
        extra-examples/html-tests/A6L-measure_test.html \
        extra-examples/html-tests/A6L-measure_test.pdf \
        extra-examples/html-tests/ANSIA-500page-ls.pdf \
        extra-examples/html-tests/ANSIA-500page-p.pdf \
        extra-examples/html-tests/ANSIA-landscape.css \
        extra-examples/html-tests/ANSIAL-measure_test.html \
        extra-examples/html-tests/ANSIAL-measure_test.pdf \
        extra-examples/html-tests/README.md \
        extra-examples/html-tests/TEST-A4.css \
        extra-examples/html-tests/gridtest \
        extra-examples/html-tests/gridtest-template.html \
        extra-examples/html-tests/measure_test.1.html \
        extra-examples/html-tests/measure_test.1.pdf \
        extra-examples/html-tests/measure_test.html \
        extra-examples/html-tests/measure_test.pdf \
        extra-examples/html-tests/measure_test_BASELINE.pdf \
        extra-examples/html-tests/measure_test_calc.pdf \
        extra-examples/html-tests/measure_test_cli.pdf \
        extra-examples/html-tests/webkit4n.pdf \
        extra-examples/html-tests/webkit6.pdf \
        extra-examples/indexing-tests/README.md \
        extra-examples/indexing-tests/index-test.html \
        extra-examples/indexing-tests/indexed.pdf \
        extra-examples/indexing-tests/noborder.pdf \
        extra-examples/indexing-tests/noindex.pdf \
        extra-examples/indexing-tests/overflowing_index_test.html \
        extra-examples/indexing-tests/overflowing_index_test.pdf \
        extra-examples/indexing-tests/test2-vertical.pdf \
        extra-examples/indexing-tests/test3-horizontal.pdf \
        extra-examples/indexing-tests/test4-horizontal-noborders.pdf \
        extra-examples/indexing-tests/test_classic.pdf \
        extra-examples/indexing-tests/test_mode_CLASSIC.pdf \
        extra-examples/indexing-tests/test_mode_ENHANCED.pdf \
        extra-examples/indexing-tests/test_mode_INDEX_CLASSIC_SIDEBAR.pdf \
        extra-examples/indexing-tests/test_mode_INDEX_OFF.pdf \
        extra-examples/indexing-tests/test_modes.html \
        extra-examples/indexing-tests/testpage.html \
        extra-examples/indexing-tests/testpage_anchor_only.html \
        src/cli++/icprint-cli \
        src/wk2gtkpdf/makefile

HEADERS += \
        src/log++/ic_printerlog++.h \
        src/wk2gtkpdf/cairo_painter.h \
        src/wk2gtkpdf/encode_image.h \
        src/wk2gtkpdf/ichtmltopdf++.h \
        src/wk2gtkpdf/ichtmltopdf_int.h \
        src/wk2gtkpdf/iclog.h \
        src/wk2gtkpdf/index_pdf.h \
        src/wk2gtkpdf/pretty_html.h
