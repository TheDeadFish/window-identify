project(WindowIdentify)
cmake_minimum_required(VERSION 3.12)

add_library( winidentify STATIC
	src/windowinfo.cpp src/winidentify.cpp 
	src/winidentify.rc)
	
add_executable(test test/hexView.cpp test/test.cpp test/test.rc)
target_link_libraries(test winidentify -lpsapi)

add_custom_command(TARGET winidentify POST_BUILD COMMAND 
	libres $<TARGET_FILE:winidentify> $<TARGET_FILE:winidentify>)
	
	