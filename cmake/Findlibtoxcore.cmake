# Try finding libtoxcore for building 

include(LibFindMacros)
libfind_pkg_check_modules(libtoxcore_PKGCONF libtoxcore)

find_path(libtoxcore_INCLUDE_DIR NAMES tox/tox.h PATHS ${libtoxcore_PKGCONF_INCLUDE_DIRS})
find_library(libtoxcore_LIBRARY NAMES toxcore PATHS ${libtoxcore_PKGCONF_LIBRARY_DIRS})

set(libtoxcore_PROCESS_INCLUDES libtoxcore_INCLUDE_DIR)
set(libtoxcore_PROCESS_LIBS libtoxcore_LIBRARY)
libfind_process(libtoxcore)
