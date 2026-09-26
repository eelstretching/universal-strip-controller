add_library(usc_status_lights status_lights.cpp light_pattern.cpp)

target_include_directories(usc_status_lights PUBLIC include)
target_compile_options(usc_status_lights PRIVATE -Wall -Wextra)

target_link_libraries(usc_status_lights
    PUBLIC
        PicoLEDs
        pico_rand
        pico_time
)
