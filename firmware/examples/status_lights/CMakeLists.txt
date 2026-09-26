add_executable(status_lights_example main.cpp)

target_link_libraries(status_lights_example
    PRIVATE
        pico_stdlib
        usc_status_lights
)

pico_enable_stdio_usb(status_lights_example 1)
pico_enable_stdio_uart(status_lights_example 0)

pico_add_extra_outputs(status_lights_example)
