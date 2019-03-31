project(Mage)
set(CMAKE_BUILD_TYPE Release)

# set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Werror")

# Reduce generated files (ZERO_CHECK in visual studio)
set(CMAKE_SUPPRESS_REGENERATION true)

# build
add_executable(mage mage.c)
