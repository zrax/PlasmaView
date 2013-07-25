project(PlasmaView)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(HSPlasma REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)

option(PlasmaView_USE_GLES "Use GLES 2.0 instead of Desktop OpenGL" OFF)
if(PlasmaView_USE_GLES)
    find_package(GLESv2 REQUIRED)
else()
    find_package(OpenGL REQUIRED)
endif()

if(MSVC)
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

if(WIN32)
    # Fix <windows.h> doing stupid stuff to QDateTime's header
    add_definitions("-DNOMINMAX")
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
    plasma_scene.cpp
    trackball.cpp
)

set(PlasmaView_MOC_Sources
    plasmaview.h
    plasma_scene.h
    trackball.h
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

# Qt5OpenGLConfig.cmake doesn't give us the GL dependencies automatically :(
target_link_libraries(PlasmaView ${OPENGL_LIBRARIES})
