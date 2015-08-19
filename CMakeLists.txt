cmake_minimum_required(VERSION 2.8)
include(cmake/defaults.cmake)

project(ShadertoyVR)

set(TARGET_NAME ${PROJECT_NAME})

find_package(Threads)
list(APPEND ${PROJECT_NAME}_LIBS ${CMAKE_THREAD_LIBS_INIT})

set(QT_COMPONENTS Core Gui OpenGL Network Qml Quick Xml)
find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
foreach(QT_MODULE ${QT_COMPONENTS})
  list(APPEND ${PROJECT_NAME}_LIBS Qt5::${QT_MODULE})
  include_directories(${Qt5${QT_MODULE}_INCLUDE_DIRS})
endforeach()

#dump_cmake_vars()

file(GLOB_RECURSE SOURCE_FILES src/*.h src/*.cpp)

include_directories(src)
include_directories(src/common)

file(GLOB_RECURSE APP_RESOURCES  res/* )
list(APPEND SOURCE_FILES ${APP_RESOURCES})

GroupSources("src")
GroupSources("res")

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
    target_link_libraries(${PROJECT_NAME} opengl32.lib)
    copy_dlls_beside_windows_executable()
elseif(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCE_FILES} )
    find_library(OpenGL OpenGL)  
else()
    add_executable(${PROJECT_NAME} ${SOURCE_FILES} )
endif()

#
# External Dependencies
#

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
add_definitions(-DGLM_FORCE_RADIANS)
include_directories(${GLM_INCLUDE_DIRS})

add_dependency_external_projects(boostconfig)
find_package(BoostConfig REQUIRED)
include_directories(${BOOSTCONFIG_INCLUDE_DIRS})

add_dependency_external_projects(oglplus)
find_package(OGLPLUS REQUIRED)
add_definitions(-DOGLPLUS_LOW_PROFILE=1)
add_definitions(-DOGLPLUS_USE_GLEW=1)
add_definitions(-DOGLPLUS_USE_BOOST_CONFIG=1)
add_definitions(-DOGLPLUS_NO_SITE_CONFIG=1)
include_directories(${OGLPLUS_INCLUDE_DIRS})

add_dependency_external_projects(glew)
find_package(GLEW REQUIRED)
add_definitions(-DGLEW_STATIC)
list(APPEND ${PROJECT_NAME}_LIBS ${GLEW_LIBRARY})
include_directories(${GLEW_INCLUDE_DIRS})


target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_LIBS)

#qt5_add_resources(COMMON_RCC Common.qrc)
#list(APPEND SOURCE_FILES ${COMMON_RCC})

