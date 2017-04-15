cmake_minimum_required(VERSION 3.5)
project(vimvs-dummy)

ucm_add_files("dummy.cpp" TO DUMMY_SRC)
ucm_add_dirs("d0" TO DUMMY_SRC RECURSIVE)
add_executable(vimvs-dummy ${DUMMY_SRC})

# By defining these macros, the Visual Studio generator picks it up, and sets the project to use Unicode
target_compile_definitions(vimvs-dummy PRIVATE UNICODE _UNICODE)

cz_set_postfix()

add_custom_command(
		TARGET vimvs-dummy POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:vimvs-dummy> ${CMAKE_SOURCE_DIR}/bin/vimvs-dummy-cl.exe
		)
add_custom_command(
		TARGET vimvs-dummy POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:vimvs-dummy> ${CMAKE_SOURCE_DIR}/bin/vimvs-dummy-lib.exe
		)
add_custom_command(
		TARGET vimvs-dummy POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:vimvs-dummy> ${CMAKE_SOURCE_DIR}/bin/vimvs-dummy-link.exe
		)

