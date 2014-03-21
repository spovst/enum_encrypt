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

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(GMP_NAME "gmp-linux32")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(GMP_NAME "gmp-win32")
endif()

add_library(${GMP_NAME} STATIC IMPORTED)
set_property(TARGET ${GMP_NAME} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/lib${GMP_NAME}.a)

add_executable(${TARGET} ${SOURCES})
target_link_libraries(${TARGET} ${GMP_NAME})