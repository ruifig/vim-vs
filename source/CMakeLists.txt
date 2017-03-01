cmake_minimum_required(VERSION 3.5)
project(vimvslib)

ucm_add_files("crazygaze/vimvs/vimvs.h" TO VIMVSLIB_SRC)
ucm_add_files("crazygaze/vimvs/vimvs.cpp" TO VIMVSLIB_SRC)
ucm_add_files("sqlite/sqlite3.h" TO VIMVSLIB_SRC)
ucm_add_files("sqlite/sqlite3.c" TO VIMVSLIB_SRC)

add_library(vimvslib STATIC ${VIMVSLIB_SRC})
target_include_directories(vimvslib PUBLIC ".")

if (MSVC)
	#
elseif (MINGW)
	#
elseif(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
	target_link_libraries(vimvslib INTERFACE pthread )
endif()

cz_set_postfix()

