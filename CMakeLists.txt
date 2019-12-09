cmake_minimum_required(VERSION 3.1)
project(Mage)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(ASM_NASM)
include_directories("include")

add_executable(moad src/loader.c)
add_executable(mink src/linker.c)

if(EXISTS "C:/Zydis/include")
	add_executable(seer src/decompiler.cpp)
	set(ZYDIS_LIB "C:/Zydis/msvc/bin/ReleaseX64")
	set_target_properties(seer PROPERTIES LINK_FLAGS "${ZYDIS_LIB}/Zydis.lib")
	include_directories("include" "C:/Zydis/include" "C:/Zydis/dependencies/zycore/include" "C:/Zydis/msvc")

	if(WIN32)
		# copy dll to output directory
		add_custom_command(TARGET seer POST_BUILD COMMAND
			${CMAKE_COMMAND} -E copy_if_different "${ZYDIS_LIB}/Zydis.dll" $<TARGET_FILE_DIR:seer>)
	endif()
endif()

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set_target_properties(moad PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

# only supports 64 bit
if(WIN32)
	set(CMAKE_ASM_NASM_OBJECT_FORMAT win64)
elseif(UNIX)
	set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
endif()

# list of projects to compile
set(PROGRAM_LIST popup print)

foreach(prog ${PROGRAM_LIST})
	file(GLOB SOURCES src/${prog}/*.asm)
	add_executable(${prog} ${SOURCES})
	add_dependencies(${prog} mink) # compile linker first

	if(true) # link to MAGE executable
		set(CMAKE_ASM_NASM_LINK_EXECUTABLE "mink <OBJECTS> -o <TARGET_NAME>.mage -l <LINK_LIBRARIES>")
	else() # link normally to PE executable
		if(WIN32)
			set(CMAKE_ASM_NASM_LINK_EXECUTABLE "link <OBJECTS> /ENTRY:main /SUBSYSTEM:WINDOWS /OUT:<TARGET> <LINK_LIBRARIES>")
		elseif(UNIX)
			# todo
		endif()
	endif()
endforeach(prog)

# project libraries
if(WIN32)
	target_link_libraries(popup user32)
	target_link_libraries(print kernel32)
elseif(UNIX)
	# todo
endif()