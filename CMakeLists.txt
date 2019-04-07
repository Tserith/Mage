project(Mage)
set(CMAKE_BUILD_TYPE Release)
enable_language(ASM_NASM)

if(WIN32)
	set(CMAKE_ASM_NASM_LINK_EXECUTABLE "link <OBJECTS> /ENTRY:start /SUBSYSTEM:WINDOWS /OUT:<TARGET> <LINK_LIBRARIES>")
elseif(UNIX)
	# need to test
endif()

add_executable(mage mage.c)

add_executable(test programs/test.asm)
target_link_libraries(test user32.lib kernel32.lib)