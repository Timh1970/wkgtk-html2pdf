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

CXX = g++

CXXFLAGS := -std=c++20 -Wall -Wextra -O2  -m64 -pedantic-errors -g

CPPFLAGS += $(USER_INCDIR)
CXXFLAGS += -DAPP_VERSION=\"$(VERSION)\"

CPPFLAGS += $(shell pkg-config --cflags wk2gtkpdf)
CPPFLAGS += $(shell pkg-config --cflags libsystemd)


LDLIBS += $(USER_LIBDIR)
LDLIBS += $(shell pkg-config --libs wk2gtkpdf)
LDLIBS += $(shell pkg-config --libs libsystemd)


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
