set (SERVICE_LIB "indicatortransferservice")
set (SERVICE_EXEC "indicator-transfer-service")

add_definitions (-DG_LOG_DOMAIN="${CMAKE_PROJECT_NAME}")

# handwritten source code...
set (SERVICE_LIB_HANDWRITTEN_SOURCES
     controller.cpp
     dm-source.cpp
     model.cpp
     plugin-source.cpp
     transfer.cpp
     view.cpp
     view-gmenu.cpp
     source.cpp
     multisource.cpp)

add_library(${SERVICE_LIB} STATIC ${SERVICE_LIB_HANDWRITTEN_SOURCES})

# add the bin dir to the include path so that
# the compiler can find the generated header files
include_directories (${CMAKE_CURRENT_BINARY_DIR})

link_directories (${SERVICE_DEPS_LIBRARY_DIRS})

set (SERVICE_EXEC_HANDWRITTEN_SOURCES main.cpp)
add_executable (${SERVICE_EXEC} ${SERVICE_EXEC_HANDWRITTEN_SOURCES})
target_link_libraries (${SERVICE_EXEC} ${SERVICE_LIB} ${SERVICE_DEPS_LIBRARIES} ${GCOV_LIBS})
install (TARGETS ${SERVICE_EXEC} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

# the DownloadManager source plugin...
set(DM_LIB "dmtransfers") 
set(DM_SOURCES dm-source.cpp dm-plugin.cpp)
add_library(${DM_LIB} MODULE ${DM_SOURCES})
target_link_libraries (${DM_LIB} ${SERVICE_LIB} ${SERVICE_DEPS_LIBRARIES} ${GCOV_LIBS})
install(TARGETS ${DM_LIB} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

# add warnings/coverage info on handwritten files
# but not the generated ones...
set_property (SOURCE ${SERVICE_LIB_HANDWRITTEN_SOURCES} ${SERVICE_EXEC_HANDWRITTEN_SOURCES} ${DM_SOURCES}
              APPEND_STRING PROPERTY COMPILE_FLAGS " -std=c++11 -g ${CXX_WARNING_ARGS} ${GCOV_FLAGS}")

set_property (SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS PLUGINDIR="${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}")
#target_compile_definitions(${DM_LIB} PUBLIC -DPLUGINDIR=${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

install (TARGETS ${SERVICE_EXEC} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})
