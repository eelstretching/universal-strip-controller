add_executable(rtc_time_example main.c)

target_link_libraries(rtc_time_example
    PRIVATE
        pico_stdlib
        usc_ds3231
)

pico_enable_stdio_usb(rtc_time_example 1)
pico_enable_stdio_uart(rtc_time_example 0)

pico_add_extra_outputs(rtc_time_example)
