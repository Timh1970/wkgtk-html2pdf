SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

USER_INCDIR ?= -I/usr/include/wk2gtkpdf
USER_LIBDIR ?=

CXX = g++
CPPFLAGS += $(USER_INCDIR)
CXXFLAGS := -std=c++20 -Wall -Wextra -O2  -m64 -pedantic-errors
CXXFLAGS += $(shell pkg-config --cflags wk2gtkpdf-6)
CXXFLAGS += $(shell pkg-config --cflags libsystemd)



LDFLAGS += $(USER_LIBDIR)
LDLIBS += $(shell pkg-config --libs wk2gtkpdf-6)
LDLIBS += $(shell pkg-config --libs libsystemd)

LDLIBS +=

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

indextest: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) indextest indexed.pdf noindex.pdf testpage_anchor_only.html


.PHONY: distclean
distclean: clean
	rm -f indextest
