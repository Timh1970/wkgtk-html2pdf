SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++
CXXFLAGS := -std=c++20 -Wall
CXXFLAGS += $(shell pkg-config --cflags webkit2gtk-4.1)

LDLIBS := -Wl,--start-group
LDLIBS += $(shell pkg-config --libs wk2gtkpdf)
LDLIBS += -Wextra -O2  -m64 -pedantic-errors -Wl,--end-group

hellopdf: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) hellopdf

install:
	install -Dm755 -t $(DESTDIR)$(BINDIR) hellopdf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/hellopdf

.PHONY: distclean
distclean: clean
	rm -f hello
