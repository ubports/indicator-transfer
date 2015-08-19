# the DownloadManager source plugin...
set(DM_LIB "dmtransfers")

set(DM_SOURCES
    dm-source.cpp
    dm-plugin.cpp)

include_directories (
    ${CAMAKE_SOURCE_DIR}/src)

add_library(${DM_LIB} MODULE ${DM_SOURCES})

target_link_libraries (${DM_LIB}
    indicator-transfer
    ${SERVICE_DEPS_LIBRARIES}
    ${GCOV_LIBS})

install(TARGETS ${DM_LIB} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

set_property (SOURCE ${DM_SOURCES}
              APPEND_STRING PROPERTY COMPILE_FLAGS " -std=c++11 -fPIC -g ${CXX_WARNING_ARGS} ${GCOV_FLAGS}")
