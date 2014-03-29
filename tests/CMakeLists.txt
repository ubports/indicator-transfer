# build libgtest
add_library (gtest STATIC 
             ${GTEST_SOURCE_DIR}/gtest-all.cc 
             ${GTEST_SOURCE_DIR}/gtest_main.cc)
set_target_properties (gtest PROPERTIES INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${GTEST_INCLUDE_DIR})
set_target_properties (gtest PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS} -w)

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${CC_WARNING_ARGS}")

# look for headers in our src dir, and also in the directories where we autogenerate files...
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (${CMAKE_CURRENT_BINARY_DIR})
include_directories (${DBUSTEST_INCLUDE_DIRS})

function(add_test_by_name name)
  set (TEST_NAME ${name})
  add_executable (${TEST_NAME} ${TEST_NAME}.cpp)
  add_test (${TEST_NAME} ${TEST_NAME})
  add_dependencies (${TEST_NAME} libindicatortransferservice)
  target_link_libraries (${TEST_NAME} indicatortransferservice gtest ${SERVICE_DEPS_LIBRARIES} ${GTEST_LIBS})
endfunction()
add_test_by_name(test-mocks)

