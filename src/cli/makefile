SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

USER_INCDIR ?= -I/usr/include/wk2gtkpdf
USER_LIBDIR ?=

CXX = g++

CXXFLAGS := -std=c++20 -Wall -Wextra -O2  -m64 -pedantic-errors

CPPFLAGS += $(USER_INCDIR)
CPPFLAGS += $(shell pkg-config --cflags wk2gtkpdf)
CPPFLAGS += $(shell pkg-config --cflags webkit2gtk-4.1)
CPPFLAGS += $(shell pkg-config --cflags libsystemd)
CPPFLAGS += $(shell pkg-config --cflags libpodofo)
CPPFLAGS += $(shell pkg-config --cflags json-c)

LDLIBS += $(USER_LIBDIR)
LDLIBS += $(shell pkg-config --libs wk2gtkpdf)
LDLIBS += $(shell pkg-config --libs webkit2gtk-4.1)
LDLIBS += $(shell pkg-config --libs libsystemd)
LDLIBS += $(shell pkg-config --libs libpodofo)
LDLIBS += $(shell pkg-config --libs json-c)

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

wkgtk-html2pdf: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f $(OBJECTS) html2pdf

install:
	install -Dm755 -t $(DESTDIR)$(BINDIR) wkgtk-html2pdf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/html2pdf

.PHONY: distclean
distclean: clean
	rm -f html2pdf
