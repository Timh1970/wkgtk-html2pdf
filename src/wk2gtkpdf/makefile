 include ../../version.conf

MAJOR := $(major)
MINOR := $(minor)
PATCH := $(patch)
VERSION := $(MAJOR).$(MINOR).$(PATCH)

SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cpp=.o)

CXX = g++

CXXFLAGS +=
CXXFLAGS += -std=c++20 -fPIC -O2 -fvisibility=hidden -g
CXXFLAGS += -DAPP_VERSION=\"$(VERSION)\"

CPPFLAGS +=
CPPFLAGS += $(shell pkg-config --cflags libpodofo)

LDFLAGS += -Wl,--version-script=libwk2gtkpdf.map

LDLIBS += $(shell pkg-config --libs libpodofo )
LDLIBS += $(shell pkg-config --libs json-c)
LDLIBS += $(shell pkg-config --libs libsystemd)
LDLIBS += $(shell pkg-config --libs x11)
LDLIBS += $(shell pkg-config --libs wayland-client)

ifneq (,$(findstring -DUSE_WEBKIT_6,$(CXXFLAGS) $(CPPFLAGS) $(DEB_CXXFLAGS_MAINT_APPEND)))
    ENGINE_SUFFIX = -6
    WEBKIT_NAME = webkitgtk-6.0
else
    ENGINE_SUFFIX = -4
    WEBKIT_NAME = webkit2gtk-4.1
endif

APPLICATION = libwk2gtkpdf$(ENGINE_SUFFIX).so
FULLNAME = $(APPLICATION).$(VERSION)
SONAME = $(APPLICATION).$(MAJOR)

CPPFLAGS += $(shell pkg-config --cflags $(WEBKIT_NAME))
LDLIBS += $(shell pkg-config --libs $(WEBKIT_NAME))

LIBDIR ?= /usr/lib

$(FULLNAME): $(OBJECTS)
	$(CXX) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(LDLIBS)
	ln -sfT $(FULLNAME) $(SONAME)
	ln -sfT $(SONAME) $(APPLICATION)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: install
install:
	# Shared Library
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(FULLNAME) $(DESTDIR)$(LIBDIR)/$(FULLNAME)
	ln -sfT $(FULLNAME) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sfT $(SONAME)   $(DESTDIR)$(LIBDIR)/$(APPLICATION)

	# Headers
	install -d $(DESTDIR)/usr/include/wk2gtkpdf
	install -m 644 *.h $(DESTDIR)/usr/include/wk2gtkpdf/

	# Assets (The CSS and JS Helpers)
	install -d $(DESTDIR)/usr/share/wk2gtkpdf
	install -m 644 ../../templates/*.css $(DESTDIR)/usr/share/wk2gtkpdf/
	install -m 644 ../../overflow-monitor.js $(DESTDIR)/usr/share/wk2gtkpdf/

	# Pkg-config (Explicitly define the local shell variable for the recipe)
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	@pc_file="wk2gtkpdf$(ENGINE_SUFFIX).pc"; \
	echo "prefix=/usr" > $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "exec_prefix=\$${prefix}" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "libdir=$(LIBDIR)" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "includedir=\$${prefix}/include" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "Name: wk2gtkpdf$(ENGINE_SUFFIX)" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "Description: Professional WebKit-based HTML to PDF engine" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "Version: $(VERSION)" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "Requires.private: $(WEBKIT_NAME) libpodofo json-c" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "Libs: -L\$${libdir} -lwk2gtkpdf$(ENGINE_SUFFIX)" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file; \
	echo "Cflags: -I\$${includedir}" >> $(DESTDIR)$(LIBDIR)/pkgconfig/$$pc_file


.PHONY: smoke-install
smoke-install:
	@stagedir=$$(mktemp -d /tmp/icframework-stage.XXXXXX); \
	echo "Using staging dir: $$stagedir"; \
	$(MAKE) install DESTDIR=$$stagedir || { echo "install failed"; rm -rf $$stagedir; exit 1; }; \
	echo "Installed files:"; ls -l $$stagedir$(LIBDIR)/${APPLICATION}* || true; \
	if [ -f $$stagedir$(LIBDIR)/$(FULLNAME) ]; then \
	  readelf -d $$stagedir$(LIBDIR)/$(FULLNAME) | grep -F SONAME || true; \
	else \
	  echo "Warning: $(FULLNAME) not found in $$stagedir$(LIBDIR)"; \
	fi; \
	# keep staging dir for inspection, or uncomment the next line to auto-clean:
	rm -rf $$stagedir

.PHONY: uninstall
uninstall:
	# Remove Library and Symlinks
	rm -f $(DESTDIR)$(LIBDIR)/$(FULLNAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(APPLICATION)

	# Remove Pkg-config
	rm -f "$(DESTDIR)$(LIBDIR)/pkgconfig/wk2gtkpdf$(ENGINE_SUFFIX).pc"

	# Remove Headers
	rm -rf $(DESTDIR)/usr/include/wk2gtkpdf

	# Remove Assets (CSS/JS Helpers)
	rm -f $(DESTDIR)/usr/share/wk2gtkpdf/*.css
	rm -f $(DESTDIR)/usr/share/wk2gtkpdf/overflow-monitor.js
	# Only remove the directory if it is empty
	rmdir --ignore-fail-on-non-empty $(DESTDIR)/usr/share/wk2gtkpdf


.PHONY: clean
clean:
	rm -f *.o $(FULLNAME) $(SONAME) $(APPLICATION)

.PHONY: distclean
distclean: clean
	rm -f $(APPLICATION) $(APPLICATION).*
