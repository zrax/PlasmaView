project(PlasmaView)
cmake_minimum_required(VERSION 2.8.11)

find_package(HSPlasma REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)

if(MSVC)
    add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()

if(WIN32)
    # Fix <windows.h> doing stupid stuff to QDateTime's header
    add_definitions("/DNOMINMAX")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS "-std=c++0x ${WARNING_FLAGS} ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${WARNING_FLAGS} ${CMAKE_C_FLAGS}")
endif()

include_directories("${PlasmaView_SOURCE_DIR}")
include_directories("${HSPlasma_INCLUDE_DIRS}")

set(PlasmaView_Sources
    main.cpp
    plasmaview.cpp
    plasma_gles.cpp
)

set(PlasmaView_MOC_Sources
    plasmaview.h
    plasma_gles.h
)
qt5_wrap_cpp(PlasmaView_MOC ${PlasmaView_MOC_Sources})

set(PlasmaView_Resources
    plasmaview.qrc
)
qt5_add_resources(PlasmaView_RCC ${PlasmaView_Resources})

if(MSVC)
    # Make these show up in the Solution Explorer
    set(PlasmaView_Sources ${PlasmaView_MOC_Sources}
                           ${PlasmaView_Sources})
endif()

add_executable(PlasmaView WIN32 MACOSX_BUNDLE
               ${PlasmaView_Sources} ${PlasmaView_MOC} ${PlasmaView_RCC})
qt5_use_modules(PlasmaView Core Widgets OpenGL)
target_link_libraries(PlasmaView HSPlasma)

# Oh look, CMake+Qt+OpenGL is STILL broken on Windows :(
if(WIN32)
    set(GLESv2_PREFIX "GLESv2_PREFIX-NOTFOUND"
        CACHE PATH "Path to libEGL and libGLESv2")

    if(GLESv2_PREFIX STREQUAL "GLESv2_PREFIX-NOTFOUND")
        message(FATAL_ERROR "GLESv2 is required, but GLESv2_PREFIX was not set")
    endif()

    target_link_libraries(PlasmaView ${GLESv2_PREFIX}/libEGL.lib
                                     ${GLESv2_PREFIX}/libGLESv2.lib)
endif()
