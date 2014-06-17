project (indicator-transfer C CXX)
cmake_minimum_required (VERSION 2.8.9)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set (PROJECT_VERSION "14.04.0")
set (PACKAGE ${CMAKE_PROJECT_NAME})
set (GETTEXT_PACKAGE "indicator-transfer")
add_definitions (-DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
                 -DGNOMELOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

option (enable_tests "Build the package's automatic tests." ON)
option (enable_lcov "Generate lcov code coverage reports." ON)

##
##  GNU standard installation directories
## 

include (GNUInstallDirs)
if (EXISTS "/etc/debian_version") # Workaround for libexecdir on debian
  set (CMAKE_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBDIR}")
  set (CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif ()
set (CMAKE_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")
set (CMAKE_INSTALL_FULL_PKGLIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")

##
##  Check for prerequisites
##

find_package (PkgConfig REQUIRED)
include (FindPkgConfig)

pkg_check_modules (SERVICE_DEPS REQUIRED
                   glib-2.0>=2.36
                   gio-unix-2.0>=2.36
                   properties-cpp>=0.0.1)
include_directories (SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})

##
##  custom targets
##

add_custom_target (cppcheck COMMAND cppcheck --enable=all -q --error-exitcode=2 -I${CMAKE_SOURCE_DIR}/include
                   ${CMAKE_SOURCE_DIR}/src
                   ${CMAKE_SOURCE_DIR}/tests)

##
##  Actually building the service
##

# those GActionEntry structs tickle -Wmissing-field-initializers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CXX_WARNING_ARGS "${CXX_WARNING_ARGS} -Weverything -Wno-c++98-compat -Wno-missing-field-initializers")
else()
  set (CXX_WARNING_ARGS "${CXX_WARNING_ARGS} -Wall -Wextra -Wpedantic -Wno-missing-field-initializers")
endif()

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)

# testing & coverage
if (${enable_tests})
  pkg_check_modules (DBUSTEST REQUIRED dbustest-1>=14.04.0)
  set (GTEST_SOURCE_DIR /usr/src/gtest/src)
  set (GTEST_INCLUDE_DIR ${GTEST_SOURCE_DIR}/..)
  set (GTEST_LIBS -lpthread)
  enable_testing ()
  if (${enable_lcov})
    include(GCov)
  endif ()
endif ()

# add the subdirs
add_subdirectory(data)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(po)
if (${enable_tests})
  add_subdirectory(tests)
endif ()

