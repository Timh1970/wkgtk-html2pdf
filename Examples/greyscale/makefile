SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++
CXXFLAGS := -std=c++20 -Wall
CXXFLAGS += $(shell pkg-config --cflags webkit2gtk-4.1)
CXXFLAGS += $(shell Magick++-config --cppflags)

LDLIBS := -Wl,--start-group
LDLIBS += $(shell pkg-config --libs wk2gtkpdf)
LDLIBS += $(shell Magick++-config --ldflags)

LDLIBS += -Wextra -O2  -m64 -pedantic-errors -Wl,--end-group

grayscale: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

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
