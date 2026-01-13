cmake_minimum_required(VERSION 3.22)
include(${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_policy(SET CMP0076 NEW)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME foc2)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME} C CXX ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})
add_executable(${CMAKE_PROJECT_NAME})

# STM32Cube paths
set(STM32_CUBE_G4_PATH /home/eug/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.1)
set(STM32_HAL_DRIVER ${STM32_CUBE_G4_PATH}/Drivers/STM32G4xx_HAL_Driver)
set(STM32_CMSIS ${STM32_CUBE_G4_PATH}/Drivers/CMSIS)
set(STM32_USB_DEVICE ${STM32_CUBE_G4_PATH}/Middlewares/ST/STM32_USB_Device_Library)

# Include paths
set(MX_Include_Dirs
    Inc
    ${STM32_HAL_DRIVER}/Inc
    ${STM32_HAL_DRIVER}/Inc/Legacy
    ${STM32_USB_DEVICE}/Core/Inc
    ${STM32_USB_DEVICE}/Class/CDC/Inc
    ${STM32_CMSIS}/Device/ST/STM32G4xx/Include
    ${STM32_CMSIS}/Include
)

# Application sources
set(MX_Application_Src
    Src/main.c
    Src/init.c
    Src/foc.c
    Src/drv/i2c_scan.c
    Src/drv/uart_in.c
    Src/drv/adc_dma.c
    Src/drv/pwm.c
    Src/drv/mt6701.c
    Src/drv/can.c
    Src/usb/usb_device.c
    Src/usb/usbd_conf.c
    Src/usb/usbd_desc.c
    Src/usb/usbd_cdc_if.c
    Src/sys/stm32g4xx_it.c
    Src/sys/stm32g4xx_hal_msp.c
    Src/sys/sysmem.c
    Src/sys/syscalls.c
    Src/sys/startup_stm32g431xx.s
)

# STM32 HAL/LL driver sources
set(STM32_Drivers_Src
    Src/sys/system_stm32g4xx.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_utils.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_exti.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_pcd.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_pcd_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_usb.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_rcc.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_rcc_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_flash.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_flash_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_flash_ramfunc.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_gpio.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_exti.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_dma.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_dma_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_pwr.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_pwr_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_cortex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_rcc.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_adc.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_adc_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_adc.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_fdcan.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_i2c.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_i2c_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_tim.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_tim_ex.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_ucpd.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_gpio.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_ll_dma.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_uart.c
    ${STM32_HAL_DRIVER}/Src/stm32g4xx_hal_uart_ex.c
)

# USB Device Library sources
set(USB_Device_Library_Src
    ${STM32_USB_DEVICE}/Core/Src/usbd_core.c
    ${STM32_USB_DEVICE}/Core/Src/usbd_ctlreq.c
    ${STM32_USB_DEVICE}/Core/Src/usbd_ioreq.c
    ${STM32_USB_DEVICE}/Class/CDC/Src/usbd_cdc.c
)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${MX_Application_Src}
    ${USB_Device_Library_Src}
    ${STM32_Drivers_Src}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${MX_Include_Dirs}
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USE_FULL_LL_DRIVER
    USE_HAL_DRIVER
    STM32G431xx
    $<$<CONFIG:Debug>:DEBUG>
)

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME}
    ${TOOLCHAIN_LINK_LIBRARIES}
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Generating .hex and .bin files and showing size info"
)
