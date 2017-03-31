cmake_minimum_required(VERSION 3.5)
project(vimvs-dummy-tool)

ucm_add_files("dummy.cpp" TO DUMMY_SRC)
ucm_add_dirs("d0" TO DUMMY_SRC RECURSIVE)
add_executable(vimvs-dummy-tool ${DUMMY_SRC})
target_include_directories(vimvs-dummy-tool PRIVATE "..\\dummy\\d0\\d1\\d2\\d3")
cz_set_postfix()

add_custom_command(
		TARGET vimvs-dummy-tool POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:vimvs-dummy-tool> ${CMAKE_SOURCE_DIR}/bin/vimvs-dummy-cl.exe
		)
add_custom_command(
		TARGET vimvs-dummy-tool POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:vimvs-dummy-tool> ${CMAKE_SOURCE_DIR}/bin/vimvs-dummy-lib.exe
		)
add_custom_command(
		TARGET vimvs-dummy-tool POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:vimvs-dummy-tool> ${CMAKE_SOURCE_DIR}/bin/vimvs-dummy-link.exe
		)

