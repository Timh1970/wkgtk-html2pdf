include ../../version.conf

MAJOR := $(major)
MINOR := $(minor)
PATCH := $(patch)
VERSION := $(MAJOR).$(MINOR).$(PATCH)
APPLICATION=libwk2gtkpdf.so
FULLNAME = $(APPLICATION).$(VERSION)
SONAME = $(APPLICATION).$(MAJOR)

SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++

CXXFLAGS += -std=c++20 -fPIC -O2 -fvisibility=hidden

USER_CXXFLAGS ?=
CPPFLAGS += $(USER_CXXFLAGS)
CPPFLAGS += $(shell pkg-config --cflags webkit2gtk-4.1)
CPPFLAGS += $(shell pkg-config --cflags libpodofo)


$(FULLNAME): $(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(SONAME) -o $@ $^
	ln -sfT $(FULLNAME) $(SONAME)
	ln -sfT $(SONAME) $(APPLICATION)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: insatll
install:
	# install the real file
	install -Dm755 -t $(DESTDIR)/usr/lib/ $(FULLNAME)
	# create symlinks explicitly in DESTDIR (safer for packaging scripts)
	ln -sfT $(FULLNAME) $(DESTDIR)/usr/lib/$(SONAME)
	ln -sfT $(SONAME)   $(DESTDIR)/usr/lib/$(APPLICATION)
	# headers
	install -Dm644 -t $(DESTDIR)/usr/include/wk2gtkpdf/ *.h
	# install pkg-config file if present
	install -Dm644 -t $(DESTDIR)/usr/lib/pkgconfig icprint.pc || true

.PHONY: smoke-install
smoke-install:
	@stagedir=$$(mktemp -d /tmp/icframework-stage.XXXXXX); \
	echo "Using staging dir: $$stagedir"; \
	$(MAKE) install DESTDIR=$$stagedir || { echo "install failed"; rm -rf $$stagedir; exit 1; }; \
	echo "Installed files:"; ls -l $$stagedir/usr/lib/${APPLICATION}* || true; \
	if [ -f $$stagedir/usr/lib/$(FULLNAME) ]; then \
	  readelf -d $$stagedir/usr/lib/$(FULLNAME) | grep -F SONAME || true; \
	else \
	  echo "Warning: $(FULLNAME) not found in $$stagedir/usr/lib"; \
	fi; \
	# keep staging dir for inspection, or uncomment the next line to auto-clean:
	rm -rf $$stagedir

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/usr/lib/$(FULLNAME)
	rm -f $(DESTDIR)/usr/lib/$(SONAME)
	rm -f $(DESTDIR)/usr/lib/$(APPLICATION)
	rm -f $(DESTDIR)/usr/include/wk2gtkpdf/*.h
	rmdir --ignore-fail-on-non-empty $(DESTDIR)/usr/include/wk2gtkpdf

.PHONY: clean
clean:
	rm -f *.o $(FULLNAME) $(SONAME) $(APPLICATION)

.PHONY: distclean
distclean: clean
	rm -f $(APPLICATION) $(APPLICATION).*
