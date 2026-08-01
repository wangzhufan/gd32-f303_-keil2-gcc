# ============================================================================
# ARM GCC Toolchain File for GD32F303CCT6 (Cortex-M4F)
#
# Usage:
#   cmake -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake
#   cmake --build build
#
# Prerequisites:
#   1. Install ARM GNU Toolchain: https://developer.arm.com/downloads/-/gnu-rm
#   2. Add toolchain bin/ to your PATH, OR set TOOLCHAIN_PREFIX below
# ============================================================================

# ---- Toolchain definition ----
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Set toolchain prefix (adjust if your toolchain is at a custom path)
if(NOT DEFINED TOOLCHAIN_PREFIX)
  set(TOOLCHAIN_PREFIX arm-none-eabi-)
endif()

# ---- Locate toolchain binaries ----
# Add common ARM GCC install paths to search list
list(APPEND CMAKE_PROGRAM_PATH
  "E:/wang-test/tools/bin"
  "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/15.3 rel1/bin"
  "C:/Program Files/Arm GNU Toolchain arm-none-eabi/15.3 rel1/bin"
)

find_program(GCC_EXECUTABLE ${TOOLCHAIN_PREFIX}gcc PATHS ${CMAKE_PROGRAM_PATH})
if(NOT GCC_EXECUTABLE)
  message(FATAL_ERROR "${TOOLCHAIN_PREFIX}gcc not found. Install ARM GNU Toolchain and add it to PATH.")
endif()
get_filename_component(GCC_DIR ${GCC_EXECUTABLE} DIRECTORY)

function(find_tc_tool VAR NAME)
  set(full_path "${GCC_DIR}/${NAME}")
  if(EXISTS "${full_path}" AND NOT IS_DIRECTORY "${full_path}")
    set(${VAR} "${full_path}" PARENT_SCOPE)
  else()
    find_program(${VAR} ${NAME} PATHS ${CMAKE_PROGRAM_PATH})
    if(NOT ${VAR})
      message(FATAL_ERROR "${NAME} not found in ${GCC_DIR} or PATH.")
    endif()
  endif()
endfunction()

find_tc_tool(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
find_tc_tool(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
find_tc_tool(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_tc_tool(CMAKE_AR           ${TOOLCHAIN_PREFIX}gcc-ar)
find_tc_tool(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
find_tc_tool(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump)
find_tc_tool(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

# ---- Do not run linker test during CMake configure ----
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ---- MCU Flags (Cortex-M4F, HW float, FPv4-SP-D16) ----
set(CPU_FLAGS
  -mcpu=cortex-m4
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
)
# Join list into a space-separated string for compiler flags
string(JOIN " " CPU_FLAGS_STR ${CPU_FLAGS})

set(CMAKE_C_FLAGS_INIT          "${CPU_FLAGS_STR} -std=c99 -Wall -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS_INIT        "${CPU_FLAGS_STR} -Wall -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS_INIT        "${CPU_FLAGS_STR} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS_STR} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs")
# Ensure FPU is enabled (needed by system_gd32f30x.c)
add_compile_definitions(__FPU_USED=1)

# ---- Debug / Release build types ----
set(CMAKE_C_FLAGS_DEBUG          "-Og -g3")
set(CMAKE_CXX_FLAGS_DEBUG        "-Og -g3")
set(CMAKE_ASM_FLAGS_DEBUG        "-g3")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

set(CMAKE_C_FLAGS_RELEASE        "-O2 -g0")
set(CMAKE_CXX_FLAGS_RELEASE      "-O2 -g0")
set(CMAKE_ASM_FLAGS_RELEASE      "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
