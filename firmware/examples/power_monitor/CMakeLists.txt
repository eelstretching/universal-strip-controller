add_executable(power_monitor_example main.cpp)

target_link_libraries(power_monitor_example
    PRIVATE
        pico_stdlib
        usc_ina226
)

pico_enable_stdio_usb(power_monitor_example 1)
pico_enable_stdio_uart(power_monitor_example 0)

pico_add_extra_outputs(power_monitor_example)
