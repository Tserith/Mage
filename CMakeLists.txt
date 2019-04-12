cmake_minimum_required(VERSION 3.1)
project(Mage)
set(CMAKE_BUILD_TYPE Release)
enable_language(ASM_NASM)
include_directories(include)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(moad src/loader.c)
add_executable(mink src/linker.c)

set(PROGRAM_LIST popup)

foreach(prog ${PROGRAM_LIST})
	file(GLOB SOURCES src/${prog}/*.asm)
	add_executable(${prog} ${SOURCES})
	#set(CMAKE_ASM_NASM_LINK_EXECUTABLE "echo pizza")

	if(WIN32)
		set(CMAKE_ASM_NASM_LINK_EXECUTABLE "link <OBJECTS> /ENTRY:start /SUBSYSTEM:WINDOWS /OUT:<TARGET> <LINK_LIBRARIES>")
		target_link_libraries(popup user32.lib kernel32.lib)
	elseif(UNIX)
		# need to test
	endif()
endforeach(prog)