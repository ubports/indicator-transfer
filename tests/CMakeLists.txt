include(FindGMock)
include(FindValgrind)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${GMOCK_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # turn off the warnings that break Google Test
  set (CXX_WARNING_ARGS "${CXX_WARNING_ARGS} -Wno-global-constructors -Wno-weak-vtables -Wno-undef -Wno-c++98-compat-pedantic -Wno-missing-noreturn -Wno-used-but-marked-unused -Wno-padded -Wno-deprecated -Wno-sign-compare -Wno-shift-sign-overflow")
endif()

add_definitions(-fPIC)
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -g ${CXX_WARNING_ARGS}")

# look for headers in our src dir, and also in the directories where we autogenerate files...
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (${CMAKE_CURRENT_BINARY_DIR})
include_directories (${DBUSTEST_INCLUDE_DIRS})

function(add_valgrind_test_by_name name)
  set (TEST_NAME ${name})
  add_executable (${TEST_NAME} ${TEST_NAME}.cpp)
  add_valgrind_test(${TEST_NAME} VALGRIND ./${TEST_NAME})
  target_link_libraries (${TEST_NAME} indicator-transfer ${SERVICE_DEPS_LIBRARIES} ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES})
endfunction()
add_valgrind_test_by_name(test-controller)
add_valgrind_test_by_name(test-multisource)
add_valgrind_test_by_name(test-plugin-source)
set(PLUGIN_NAME "mock-source-plugin")
add_library(${PLUGIN_NAME} STATIC mock-source-plugin.cpp)
target_link_libraries(${PLUGIN_NAME} PRIVATE ${SERVICE_DEPS_LIBRARIES} ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES})

function(add_test_by_name name)
  set (TEST_NAME ${name})
  add_executable (${TEST_NAME} ${TEST_NAME}.cpp)
  add_test (${TEST_NAME} ${TEST_NAME})
  target_link_libraries (${TEST_NAME} indicator-transfer ${SERVICE_DEPS_LIBRARIES} ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES})
endfunction()
add_test_by_name(test-view-gmenu)

#add_test_by_name(test-mocks)
#add_test_by_name(test-gactions)
#add_test_by_name(test-actions-live)
#add_test_by_name(test-menus)
#add_test_by_name(test-exporter)

add_test (cppcheck cppcheck --enable=all -q --error-exitcode=2 --inline-suppr -I${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)

