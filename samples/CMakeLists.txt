set(ANSI_SAMPLE_SOURCES
    01_hello
    02_triangle
    03_cube
    04_snake
    05_image
    06_obj
    07_raymarch
    08_fluid
)

foreach(sample IN LISTS ANSI_SAMPLE_SOURCES)
    add_executable(${sample} ${sample}.cpp)
    target_link_libraries(${sample} PRIVATE ansi)
    target_compile_definitions(${sample} PRIVATE ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
endforeach()

target_link_libraries(05_image PRIVATE stb::stb)
target_link_libraries(06_obj PRIVATE tinyobjloader)
