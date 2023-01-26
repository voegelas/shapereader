EXTRA_DIST = CONTRIBUTING.md Doxyfile Makefile.PL shapereader.bib

SUBDIRS = tests

pkgconfig_DATA = shapereader.pc

lib_LIBRARIES = libshapereader.a
pkginclude_HEADERS = dbf.h shp-box.h shp-multipoint.h shp-point.h \
		     shp-polygon.h shp.h shx.h shapereader.h
noinst_HEADERS = convert.h
libshapereader_a_SOURCES = convert.c dbf.c shp-box.c shp-multipoint.c \
			   shp-point.c shp-polygon.c shp.c shx.c
