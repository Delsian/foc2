set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PREFIX "arm-none-eabi-")
# Set toolchain
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}size")

# Tell CMake the compilers are working (skip compiler tests)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

# Disable compiler testing entirely
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_ASM_COMPILER_FORCED TRUE)

# CPU and FPU settings
set(CPU_FLAGS "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard")

# Compiler flags
set(COMMON_FLAGS "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall -Wextra -Wno-unused-parameter")

set(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=gnu++17 -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp")

# Debug/Release specific flags
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")

# Set Release as default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Set the ASM standard
set(CMAKE_ASM_STANDARD 11)
set(CMAKE_ASM_STANDARD_REQUIRED ON)
# Set the C++ standard (if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Linker flags
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32G431XX_FLASH.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -T${LINKER_SCRIPT} -Wl,-Map=${CMAKE_PROJECT_NAME}.map,--cref -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs")
set(TOOLCHAIN_LINK_LIBRARIES "-lc -lm -lnosys")