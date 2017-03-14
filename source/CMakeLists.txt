cmake_minimum_required(VERSION 3.5)
project(vimvs)

ucm_add_files(
	"3rdparty/json.hpp"
	"ChildProcessLauncher.cpp"
	"ChildProcessLauncher.h"
	"Database.h"
	"IniFile.cpp"
	"IniFile.h"
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

# By defining these macros, the Visual Studio generator picks it up, and sets the project to use Unicode
target_compile_definitions(vimvs PRIVATE UNICODE _UNICODE)

target_link_libraries(vimvs Psapi.lib)

cz_set_postfix()

