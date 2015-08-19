set (SERVICE_LIB "indicator-transfer")
set (SERVICE_EXEC "indicator-transfer-service")

add_definitions (-DG_LOG_DOMAIN="${CMAKE_PROJECT_NAME}")

# handwritten source code...
set (SERVICE_LIB_HANDWRITTEN_SOURCES
     controller.cpp
     model.cpp
     plugin-source.cpp
     transfer.cpp
     view.cpp
     view-gmenu.cpp
     source.cpp
     multisource.cpp)

add_library(${SERVICE_LIB} SHARED ${SERVICE_LIB_HANDWRITTEN_SOURCES})
target_link_libraries (${SERVICE_LIB} PRIVATE ${SERVICE_DEPS_LIBRARIES} ${GCOV_LIBS})

set_target_properties(${SERVICE_LIB} PROPERTIES
  VERSION ${INDICATOR_TRANSFER_VERSION}
  SOVERSION ${INDICATOR_TRANSFER_VERSION_MAJOR}
)
install (TARGETS ${SERVICE_LIB} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})


# add the bin dir to the include path so that
# the compiler can find the generated header files
include_directories (${CMAKE_CURRENT_BINARY_DIR})

link_directories (${SERVICE_DEPS_LIBRARY_DIRS})

set (SERVICE_EXEC_HANDWRITTEN_SOURCES main.cpp)
add_executable (${SERVICE_EXEC} ${SERVICE_EXEC_HANDWRITTEN_SOURCES})
target_link_libraries (${SERVICE_EXEC} ${SERVICE_LIB} ${SERVICE_DEPS_LIBRARIES} ${GCOV_LIBS})
install (TARGETS ${SERVICE_EXEC} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

# add warnings/coverage info on handwritten files
# but not the generated ones...
set_property (SOURCE ${SERVICE_LIB_HANDWRITTEN_SOURCES} ${SERVICE_EXEC_HANDWRITTEN_SOURCES}
              APPEND_STRING PROPERTY COMPILE_FLAGS " -std=c++11 -fPIC -g ${CXX_WARNING_ARGS} ${GCOV_FLAGS}")

set_property (SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS PLUGINDIR="${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}")

add_subdirectory (dm-plugin)
