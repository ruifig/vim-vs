cmake_minimum_required(VERSION 3.5)
project(vimvs)

ucm_add_files(
	"3rdparty/json.hpp"
	"Logging.cpp"
	"Logging.h"
	"Parameters.cpp"
	"Parameters.h"
	"Parser.cpp"
	"Parser.h"
	"targetver.h"
	"Utils.cpp"
	"Utils.h"
	"vimvs.cpp"
	 TO VIMVS_SRC
	 )

ADD_MSVC_PRECOMPILED_HEADER("vimvsPCH.h" "vimvsPCH.cpp" VIMVS_SRC)

ucm_add_files(
	"3rdparty/sqlite/sqlite3.h"
	"3rdparty/sqlite/sqlite3.c"
	TO VIMVS_SRC
	)

add_executable(vimvs ${VIMVS_SRC})
cz_set_postfix()
cz_add_common_libs()
target_compile_definitions(vimvs PRIVATE NOMINMAX)

if (MSVC)
	#
elseif (MINGW)
	#
elseif(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
	target_link_libraries(vimvslib INTERFACE pthread )
endif()

cz_set_postfix()

