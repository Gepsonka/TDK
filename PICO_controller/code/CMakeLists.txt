add_executable(main
        main.c
        include/network.h source/network.c include/lora.h source/lora.c include/servo.h source/servo.c include/motor.h source/motor.c source/elevon.c include/elevon.h)

target_include_directories(main PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

# pull in common dependencies
target_link_libraries(main pico_stdlib hardware_spi hardware_pwm)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

# add url via pico_set_program_url
example_auto_set_url(main)
