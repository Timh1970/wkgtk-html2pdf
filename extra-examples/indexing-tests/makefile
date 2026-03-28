SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

USER_INCDIR ?= -I/usr/include/wk2gtkpdf
USER_LIBDIR ?=

CXX = g++
CPPFLAGS += $(USER_INCDIR)
CXXFLAGS := -std=c++20 -Wall
CXXFLAGS += $(shell pkg-config --cflags webkit2gtk-4.1)
CXXFLAGS += $(shell pkg-config --cflags libsystemd)
CXXFLAGS += $(shell pkg-config --cflags libpodofo)
CXXFLAGS += $(shell pkg-config --cflags json-c)


LDFLAGS += $(USER_LIBDIR)
LDLIBS := -Wl,--start-group -lwk2gtkpdf -lmupdf
LDLIBS += $(shell pkg-config --libs webkit2gtk-4.1)
LDLIBS += $(shell pkg-config --libs libsystemd)
LDLIBS += $(shell pkg-config --libs libpodofo)
LDLIBS += $(shell pkg-config --libs json-c)
LDLIBS += -Wextra -O2  -m64 -pedantic-errors -Wl,--end-group

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

indextest: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) indextest

install:
	install -Dm755 -t $(DESTDIR)$(BINDIR) indextest

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/indextest

.PHONY: distclean
distclean: clean
	rm -f indextest
