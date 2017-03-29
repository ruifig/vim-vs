cmake_minimum_required(VERSION 3.5)
project(tests)

ucm_add_files(
	"targetver.h"
	"tests.cpp"
	"tests_vimvs.cpp"
	TO TESTS_SRC
	)

ADD_MSVC_PRECOMPILED_HEADER("testsPCH.h" "testsPCH.cpp" TESTS_SRC)

add_executable(tests ${TESTS_SRC})
cz_set_postfix()
cz_add_common_libs()
target_link_libraries(tests UnitTest++)

