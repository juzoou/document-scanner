cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DocScanner LANGUAGES CXX)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist/${CMAKE_PROJECT_NAME}")
set(CMAKE_INSTALL_BINDIR ".")
set(CMAKE_INSTALL_LIBDIR ".")

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wxBUILD_SHARED OFF)
set(wxBUILD_INSTALL OFF)

FetchContent_Declare(
   wxWidgets
   GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
   GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)

set(SRCS src/main.cpp src/scan.cpp)

add_executable(${PROJECT_NAME} WIN32 ${SRCS}
   inc/common.h)

set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/opencv/build")
find_package(OpenCV REQUIRED NO_DEFAULT_PATH)
target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    wx::core
    wx::net
    wx::base
    ${OpenCV_LIBS}
)
