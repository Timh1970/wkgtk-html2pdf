include ../../version.conf

MAJOR := $(major)
MINOR := $(minor)
PATCH := $(patch)
VERSION := $(MAJOR).$(MINOR).$(PATCH)

SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

USER_INCDIR ?= -I/usr/include/wk2gtkpdf
USER_LIBDIR ?=

CXXFLAGS += -DAPP_VERSION=\"$(VERSION)\"

# Detect Engine
ifneq (,$(findstring -DUSE_WEBKIT_6,$(CXXFLAGS) $(CPPFLAGS) $(DEB_CXXFLAGS_MAINT_APPEND)))
	ENGINE_SUFFIX = -6
else
	ENGINE_SUFFIX = -4
endif

# ONLY evaluate these if we aren't cleaning
ifeq (,$(filter clean distclean uninstall,$(MAKECMDGOALS)))

    PC_NAME = wk2gtkpdf$(ENGINE_SUFFIX)

    # Use = instead of += for the first assignment to ensure they aren't empty strings
    # and remove the 2>/dev/null so you can actually see if pkg-config fails
    PKG_CPP := $(shell pkg-config --cflags $(PC_NAME) libsystemd)
    PKG_LIB := $(shell pkg-config --libs $(PC_NAME) libsystemd)

    CPPFLAGS += $(USER_INCDIR) $(PKG_CPP)
    LDLIBS   += $(USER_LIBDIR) $(PKG_LIB)

endif

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

wkgtk-html2pdf: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f $(OBJECTS) wkgtk-html2pdf

install:
	install -Dm755 -t $(DESTDIR)$(BINDIR) wkgtk-html2pdf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/wkgtk-html2pdf

.PHONY: distclean
distclean: clean
	rm -f wkgtk-html2pdf
