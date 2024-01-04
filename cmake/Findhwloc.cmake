# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(hwloc_PC_STATIC_LIBS)
    set(_hwloc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(hwloc_PC hwloc)

find_library(hwloc_LIBRARY
             NAMES hwloc
             HINTS
             ${hwloc_PC_LIBDIR}
             ${hwloc_PC_LIBRARY_DIRS})

find_path(hwloc_INCLUDE_DIR
          NAMES hwloc.h
          HINTS
          ${hwloc_PC_INCLUDEDIR}
          ${hwloc_PC_INCLUDEDIRS})

mark_as_advanced(
        hwloc_LIBRARY
        hwloc_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(hwloc
                                  REQUIRED_VARS
                                  hwloc_LIBRARY
                                  hwloc_INCLUDE_DIR
                                  VERSION_VAR hwloc_PC_VERSION)

set(hwloc_LIBRARIES ${hwloc_LIBRARY})
set(hwloc_INCLUDE_DIRS ${hwloc_INCLUDE_DIR})

if(hwloc_FOUND AND NOT (TARGET hwloc::hwloc))
    add_library(hwloc::hwloc UNKNOWN IMPORTED)

    set_target_properties(hwloc::hwloc
                          PROPERTIES
                          IMPORTED_LOCATION ${hwloc_LIBRARY}
                          INTERFACE_INCLUDE_DIRECTORIES ${hwloc_INCLUDE_DIRS})
endif()

# Restore the original find library ordering
if(hwloc_PC_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_hwloc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

