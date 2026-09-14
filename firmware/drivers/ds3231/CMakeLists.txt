add_library(usc_ds3231 ds3231.cpp)

target_include_directories(usc_ds3231 PUBLIC include)
target_compile_options(usc_ds3231 PRIVATE -Wall -Wextra)

target_link_libraries(usc_ds3231
    PUBLIC
        hardware_i2c
        pico_util
)
