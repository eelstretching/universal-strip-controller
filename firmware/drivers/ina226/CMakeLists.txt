add_library(usc_ina226 ina226.c)

target_include_directories(usc_ina226 PUBLIC include)
target_compile_options(usc_ina226 PRIVATE -Wall -Wextra)

target_link_libraries(usc_ina226 PUBLIC hardware_i2c)
