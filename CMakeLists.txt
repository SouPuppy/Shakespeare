cmake_minimum_required(VERSION 3.20)
project(Shakespeare LANGUAGES CXX)

add_executable(editor main.cpp)
target_compile_features(editor PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(editor PRIVATE /W4 /permissive-)
endif()
