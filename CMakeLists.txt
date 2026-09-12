cmake_minimum_required(VERSION 3.20)
project(Shakespeare LANGUAGES CXX)

add_executable(shakespeare main.cpp)
target_compile_features(shakespeare PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(shakespeare PRIVATE /W4 /permissive-)
endif()
