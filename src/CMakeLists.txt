set (SERVICE_LIB "indicatortransferservice")
set (SERVICE_EXEC "indicator-transfer-service")

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${CXX_WARNING_ARGS} -Wno-missing-field-initializers ${GCOV_FLAGS}")

add_definitions (-DG_LOG_DOMAIN="Indicator-Transfer")

add_library (${SERVICE_LIB} STATIC
             exporter.cpp
             gactions.cpp
             menu.cpp
             transfer-mock.cpp
             transfer-source.cpp
             transfer-source-mock.cpp)
include_directories (${CMAKE_SOURCE_DIR})
link_directories (${SERVICE_DEPS_LIBRARY_DIRS})

add_executable (${SERVICE_EXEC} main.cpp)
target_link_libraries (${SERVICE_EXEC} ${SERVICE_LIB} ${SERVICE_DEPS_LIBRARIES} ${GCOV_LIBS})
install (TARGETS ${SERVICE_EXEC} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

