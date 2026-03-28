SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2  -m64 -pedantic-errors
CPPFLAGS += $(shell pkg-config --cflags wk2gtkpdf-6)

LDLIBS += $(shell pkg-config --libs wk2gtkpdf-6)

hellopdf: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

.PHONY: clean
clean:
	rm -f hellopdf.html
	rm -f hello.pdf
	rm -f $(OBJECTS)
	rm -f hellopdf

.PHONY: distclean
distclean: clean
	rm -f hello
