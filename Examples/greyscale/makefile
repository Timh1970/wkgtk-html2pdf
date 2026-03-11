SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++
CXXFLAGS := -std=c++20 -Wall
CXXFLAGS += $(shell pkg-config --cflags wk2gtkpdf)
CXXFLAGS += $(shell pkg-config --cflags Magick++)


LDLIBS += $(shell pkg-config --libs wk2gtkpdf)
LDLIBS += $(shell pkg-config --libs Magick++)

LDLIBS += -Wextra -O2  -m64 -pedantic-errors

grayscale: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) grayscale

install:
	install -Dm755 -t $(DESTDIR)$(BINDIR) grayscale

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/grayscale

.PHONY: distclean
distclean: clean
	rm -f hello
