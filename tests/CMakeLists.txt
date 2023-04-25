set(tests
  byteorder
  dbf
  point
  pointm
  pointz
  multipoint
  multipointm
  multipointz
  polyline
  polylinem
  polylinez
  polygon
  polygonm
  polygonz
  multipatch
)

foreach(name ${tests})
  add_executable(test_${name} test_${name}.c)
  target_link_libraries(test_${name} shapereader)
  add_test(NAME test_${name}
    COMMAND test_${name}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data"
  )
endforeach()
