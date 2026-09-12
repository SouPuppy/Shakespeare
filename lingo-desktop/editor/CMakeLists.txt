add_executable(lingo-editor src/main.cpp)
set_target_properties(lingo-editor PROPERTIES OUTPUT_NAME lingo-editor)
target_compile_features(lingo-editor PRIVATE cxx_std_17)
target_link_libraries(lingo-editor PRIVATE lingo-core)
target_include_directories(lingo-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/lingo/editor)

if(MSVC)
  target_compile_options(lingo-editor PRIVATE /W4 /permissive-)
endif()
