add_library(lingo-core STATIC src/core.cpp)
target_compile_features(lingo-core PUBLIC cxx_std_17)
target_include_directories(lingo-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
  target_compile_options(lingo-core PRIVATE /W4 /permissive-)
endif()
