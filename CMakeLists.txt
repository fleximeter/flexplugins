# This file (and the other CMakeLists.txt files in this repo)
# are adapted from the prototype CMakeLists.txt provided with the SuperCollider plugin starter code.

cmake_minimum_required(VERSION 3.5)
project(fplugins)

# The SC_PATH is the path to the SuperCollider source code, and is needed for building libraries.
set(SC_PATH $ENV{SC_PATH} CACHE STRING "Path to SuperCollider source")
include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/common)

# This is helpful for clangd with CPM
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/CPM.cmake)  # for rubberband source

set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

option(SUPERNOVA "Build plugins for supernova" OFF)
if (SUPERNOVA)
    include_directories(${SC_PATH}/external_libraries/nova-tt)
    # actually just boost.atomic
    include_directories(${SC_PATH}/external_libraries/boost)
    include_directories(${SC_PATH}/external_libraries/boost_lockfree)
    include_directories(${SC_PATH}/external_libraries/boost-lockfree)
endif()

option(CPP11 "Build with c++11." ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_COMPILER_IS_CLANG 1)
endif()

# The below monstrosity sets compiler flags for SSE (Streaming SIMD Extensions). Among many other things, SSE protects
# against denormal numbers, which are known to cause glitchy audio in some cases.

# Some processors do not support SSE and will produce errors if you try to add SSE flags, so we also have to introduce
# conditional logic to guard against those errors.

# It's a real mess, and generally there's no need to touch it :)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
    add_definitions(-fvisibility=hidden)

    include (CheckCCompilerFlag)
    include (CheckCXXCompilerFlag)

    check_c_compiler_flag(-msse HAS_SSE)
    check_cxx_compiler_flag(-msse HAS_CXX_SSE)

    if (HAS_SSE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse")
    endif()
    if (HAS_CXX_SSE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse")
    endif()

    check_c_compiler_flag(-msse2 HAS_SSE2)
    check_cxx_compiler_flag(-msse2 HAS_CXX_SSE2)

    if (HAS_SSE2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
    endif()
    if (HAS_CXX_SSE2)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    endif()

    check_c_compiler_flag(-mfpmath=sse HAS_FPMATH_SSE)
    check_cxx_compiler_flag(-mfpmath=sse HAS_CXX_FPMATH_SSE)

    if (HAS_FPMATH_SSE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
    endif()
    if (HAS_CXX_FPMATH_SSE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
    endif()

    if(NATIVE)
        add_definitions(-march=native)
    endif()

    # Plugin headers require C++11, which must be explicitly enabled for gcc and clang.
    # UPDATED for C++14
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    if(CMAKE_COMPILER_IS_CLANG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()
if(MINGW)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
endif()

# For installation
if(NOT DEFINED EXTENSIONS_DIR)
    if (WIN32)
        set(EXTENSIONS_DIR %LOCALAPPDATA%/SuperCollider/Extensions)
    elseif (APPLE)
        set(EXTENSIONS_DIR "~/Library/Application Support/SuperCollider/Extensions")
    elseif (UNIX)
        set(EXTENSIONS_DIR ~/.local/share/SuperCollider/Extensions)
    endif()
endif()

add_subdirectory(src/generators)
add_subdirectory(src/pv)
add_subdirectory(src/rubberband)

install(TARGETS pv rubberband
    LIBRARY DESTINATION ${EXTENSIONS_DIR}/Jeff/Plugins
)
install(FILES 
    src/generators/generators.sc
    src/pv/pv.sc
    src/rubberband/rubberband.sc
    DESTINATION ${EXTENSIONS_DIR}/Jeff/Classes
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES
    src/generators/LoopPhasor.schelp
    src/pv/PV_CFreeze.schelp
    src/pv/PV_MagMirror.schelp
    src/pv/PV_MagSqueeze.schelp
    src/pv/PV_MagXFade.schelp
    src/rubberband/RubberBandPS.schelp
    src/rubberband/RubberBandStretcher.schelp
    DESTINATION ${EXTENSIONS_DIR}/Jeff/HelpSource/Classes
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
if(SUPERNOVA)
    install(TARGETS generators_supernova pv_supernova rubberband_supernova
        LIBRARY DESTINATION ${EXTENSIONS_DIR}/Jeff/Plugins
    )
endif()