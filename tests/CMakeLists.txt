set(tests
  convert
  dbf
  box
  point
  multipoint
  polyline
  polygon
)

foreach(name ${tests})
  add_executable(test_${name} test_${name}.c)
  target_link_libraries(test_${name} shapereader)
  add_test(NAME test_${name} COMMAND test_${name})
  set_property(TEST test_${name}
    PROPERTY ENVIRONMENT "testdatadir=${CMAKE_CURRENT_SOURCE_DIR}/data"
  )
endforeach()
