cmake_minimum_required(VERSION 3.0)
project(tess_fitslc_export VERSION 0.1
                  DESCRIPTION "Export TESS SPOC Fits light curve data")

set(CMAKE_BUILD_TYPE DEBUG)

# add the source files needed for main executable
add_executable(tess_fitslc_export
        src/tess_fitslc_export.c)

# find the cfitsio library
#find_library(CFITSIO_LIBRARY libcfitsio.a PATHS /Users/cjburke/Work/fits-wasm/cfitsio-4.1.0/build)
#message(STATUS ${CFITSIO_LIBRARY})
#if(CFITSIO_LIBRARY)
#    # link target with cfitsio library
#target_link_libraries(tess_fitslc_export PUBLIC ${CFITSIO_LIBRARY})
target_link_libraries(tess_fitslc_export PRIVATE /Users/cjburke/Work/fits-wasm/cfitsio-4.1.0/libcfitsio.a)
target_include_directories(tess_fitslc_export PUBLIC /Users/cjburke/Work/fits-wasm/cfitsio-4.1.0)
#endif()

# find the z library
#find_library(ZLIB_LIBRARY z HINT /Users/cjburke/Work/fits-wasm/zlib-1.2.12/build)
#if(ZLIB_LIBRARY)
    # link target with math library
#    target_link_libraries(tess_fitslc_export PUBLIC ${ZLIB_LIBRARY})
#endif()
target_link_libraries(tess_fitslc_export PRIVATE /Users/cjburke/Work/fits-wasm/zlib-1.2.12/libz.a)

# define header file path
target_include_directories(tess_fitslc_export PRIVATE include)

# Make sure the tess_fitslc_export function is exported along with main for javascript
#  visibility NOTE the leading '_'
target_link_options(tess_fitslc_export PRIVATE -sEXPORTED_FUNCTIONS=['_tess_fitslc_export','_tess_fitslc_getnrows','_anyfits_getheader'] -sFORCE_FILESYSTEM -sEXPORTED_RUNTIME_METHODS=FS,cwrap -sALLOW_MEMORY_GROWTH)

# output wasm and js hopefully
set(CMAKE_EXECUTABLE_SUFFIX ".html")

install(FILES ${CMAKE_BINARY_DIR}/tess_fitslc_export.js ${CMAKE_BINARY_DIR}/tess_fitslc_export.wasm DESTINATION ${CMAKE_SOURCE_DIR}/public)

message(STATUS ${CMAKE_SYSTEM_PROCESSOR})
