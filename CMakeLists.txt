cmake_minimum_required(VERSION 3.16)
project(qingtu-qt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
  add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

add_executable(qingtu-qt
  src/main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/image_job.cpp
)

target_include_directories(qingtu-qt PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
target_link_libraries(qingtu-qt PRIVATE Qt6::Widgets Qt6::Concurrent)

if(WIN32)
  set_target_properties(qingtu-qt PROPERTIES WIN32_EXECUTABLE ON OUTPUT_NAME "qingtu-qt")
endif()
