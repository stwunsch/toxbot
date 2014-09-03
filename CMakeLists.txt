# Setup cmake
cmake_minimum_required(VERSION 2.6)

# Set version number
set(Tutorial_VERSION_MAJOR 1)
set(Tutorial_VERSION_MINOR 0)

# Setup project
project(Toxbot C)

# Find dependencies
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake) # set module path
find_package(libtoxcore REQUIRED)

# Add executable
add_executable(Toxbot toxbot.c)

# Setup the include and linker paths
include_directories(
    ${libtoxcore_INCLUDE_DIRS}
)

set(LIBS ${LIBS} ${libtoxcore_LIBRARIES})

# Link libraries
target_link_libraries(Toxbot ${LIBS})
