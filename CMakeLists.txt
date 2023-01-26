cmake_minimum_required(VERSION 3.4)

project(shapereader
  LANGUAGES C
  VERSION 0.0.1
)

include(GNUInstallDirs)

set(libshapereader_a_SOURCES
  convert.c
  dbf.c
  shp-box.c
  shp-multipoint.c
  shp-point.c
  shp-polygon.c
  shp-polyline.c
  shp.c
  shx.c
)

set(pkginclude_HEADERS
  dbf.h
  shp-box.h
  shp-multipoint.h
  shp-point.h
  shp-polygon.h
  shp-polyline.h
  shp.h
  shx.h
  shapereader.h
)

add_library(shapereader STATIC
  ${libshapereader_a_SOURCES}
)

set_target_properties(shapereader PROPERTIES
  PUBLIC_HEADER "${pkginclude_HEADERS}"
)

include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
  add_definitions(-DWORDS_BIGENDIAN=1)
endif()

install(TARGETS shapereader
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shapereader
)

# Create shapereader.pc for pkg-config
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir \${exec_prefix}/${CMAKE_INSTALL_LIBDIR})
set(includedir \${prefix}/${CMAKE_INSTALL_INCLUDEDIR})
set(PACKAGE_VERSION ${shapereader_VERSION})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shapereader.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/shapereader.pc
  @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/shapereader.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
