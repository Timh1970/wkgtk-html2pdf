SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++
USER_CXXFLAGS ?=
CXXFLAGS += $(USER_CXXFLAGS)
CXXFLAGS += -std=c++17 -fPIC -O2 -fvisibility=hidden
CXXFLAGS += $(shell pkg-config --cflags webkit2gtk-4.1)

APPLICATION=libwk2gtkpdf.so

$(APPLICATION): $(OBJECTS)
	$(CXX) -shared -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

install:
	install -Dm755 -t $(DESTDIR)/usr/lib/ $(APPLICATION)
	install -Dm644 -t $(DESTDIR)/usr/include/wk2gtkpdf/ *.h

uninstall:
	rm -f $(DESTDIR)/usr/lib/$(APPLICATION)
	rm -f $(DESTDIR)/usr/include/wk2gtkpdf/*.h
	rmdir --ignore-fail-on-non-empty $(DESTDIR)/usr/include/wk2gtkpdf

.PHONY: clean
clean:
	rm -f *.o $(APPLICATION) 

.PHONY: distclean
distclean: clean
	rm -f $(APPLICATION)
