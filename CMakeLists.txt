project(Mage)
set(CMAKE_BUILD_TYPE Release)

# set compiler flags
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Werror")

# build
add_executable(mage mage.c)