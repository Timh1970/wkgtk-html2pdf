TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += \
        /usr/include/ImageMagick-7 \
        /usr/include/podofo-0.9 \
        /usr/include/libxml2 \
        /usr/include/glib-2.0 \
        /usr/include/gtk-3.0 \
        /usr/include/pango-1.0 \
        /usr/lib/glib-2.0/include \
        /usr/include/harfbuzz \
        /usr/include/cairo \
        /usr/include/gdk-pixbuf-2.0 \
        /usr/include/atk-1.0 \
        /usr/include/libsoup-3.0 \
        /usr/include/webkitgtk-4.1


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

SOURCES += \
        Examples/GTK/test_gtk3.cpp \
        Examples/GTK/test_gtk4.cpp \
        Examples/GTK3/viewer_gtk3.cpp \
        Examples/GTK4/test_gtk4.cpp \
        Examples/demo-form/demo_jobsheet.cpp \
        Examples/demo_jobsheet.cpp \
        Examples/greyscale/greyscale.cpp \
        Examples/hello-page/hello_pdf.cpp \
        Examples/indexing-tests/indextest.cpp \
        misc/template_maker/template_maker.cpp \
        src/cli++/main.cpp \
        src/cli/main.cpp \
        src/log++/ic_printerlog++.cpp \
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
        src/cli++/icprint-cli \
        src/wk2gtkpdf/makefile

HEADERS += \
        src/log++/ic_printerlog++.h \
        src/wk2gtkpdf/encode_image.h \
        src/wk2gtkpdf/ichtmltopdf++.h \
        src/wk2gtkpdf/ichtmltopdf_int.h \
        src/wk2gtkpdf/iclog.h \
        src/wk2gtkpdf/index_pdf.h \
        src/wk2gtkpdf/pretty_html.h
