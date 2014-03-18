cmake_minimum_required(VERSION 2.8)

project(enumerative_encryption)

include_directories(src include)
aux_source_directory(src SOURCES)

set(TARGET ee)
set(OPT_LVL "2")
set(LANG_STD "c99")
set(ARCH "32")

set(CMAKE_C_FLAGS "-Wall -pedantic -funsigned-char -O${OPT_LVL} -std=${LANG_STD} -m${ARCH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_library(gmp STATIC IMPORTED)
set_property(TARGET gmp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libgmp.a)

add_executable(${TARGET} ${SOURCES})
target_link_libraries(${TARGET} gmp)
