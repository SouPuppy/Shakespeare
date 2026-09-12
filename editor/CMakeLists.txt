add_executable(lingo-editor ../main.cpp)
set_target_properties(lingo-editor PROPERTIES OUTPUT_NAME lingo)
target_compile_features(lingo-editor PRIVATE cxx_std_17)
target_link_libraries(lingo-editor PRIVATE lingo-core)

if(MSVC)
  target_compile_options(lingo-editor PRIVATE /W4 /permissive-)
endif()
