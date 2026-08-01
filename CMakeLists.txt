# ============================================================================
# GD32F303CCT6 Project — CMake Build System
# ============================================================================
cmake_minimum_required(VERSION 3.20)

# ---- Project metadata ----
project(GD32F303CCT6_Pro
  VERSION     1.7.3
  DESCRIPTION "GD32F303CCT6 PD/PPS Fast Charging Test Firmware"
  LANGUAGES   C ASM
)

# ---- Optional feature switches (enable with -DCONFIG_XXX=ON) ----
option(CONFIG_TCPC_FUNC     "Enable USB PD / Type-C protocol stack"   OFF)
option(CONFIG_SCP_FUNC      "Enable SCP fast charging protocol"       OFF)
option(CONFIG_UFCS_FUNC     "Enable UFCS fast charging protocol"      OFF)
option(CONFIG_UFCS_SIMULATOR "Enable UFCS simulator mode"              OFF)
option(CONFIG_USBD_SUPPORT  "Enable USB Device HID support"           OFF)
option(ENABLE_HEX           "Generate .hex file"                      ON)
option(ENABLE_BIN           "Generate .bin file"                      ON)

# ---- MCU / Device defines ----
set(DEVICE_DEFINES
  GD32F30X_HD
  USE_STDPERIPH_DRIVER
)

# ---- Activate optional feature defines ----
if(CONFIG_TCPC_FUNC)
  list(APPEND DEVICE_DEFINES CONFIG_TCPC_FUNC)
endif()
if(CONFIG_SCP_FUNC)
  list(APPEND DEVICE_DEFINES CONFIG_SCP_FUNC)
endif()
if(CONFIG_UFCS_FUNC)
  list(APPEND DEVICE_DEFINES CONFIG_UFCS_FUNC)
endif()
if(CONFIG_UFCS_SIMULATOR)
  list(APPEND DEVICE_DEFINES CONFIG_UFCS_SIMULATOR)
endif()
if(CONFIG_USBD_SUPPORT)
  list(APPEND DEVICE_DEFINES CONFIG_USBD_SUPPORT)
endif()

# ---- Common include directories ----
set(COMMON_INCLUDES
  ${CMAKE_SOURCE_DIR}/CMSIS
  ${CMAKE_SOURCE_DIR}/FWLIB/Include
  ${CMAKE_SOURCE_DIR}/FWLIB/Source
  ${CMAKE_SOURCE_DIR}/User
  ${CMAKE_SOURCE_DIR}/HardWare/COMMON
  ${CMAKE_SOURCE_DIR}/HardWare/I2C
  ${CMAKE_SOURCE_DIR}/CM_BACKTRACE
  ${CMAKE_SOURCE_DIR}/CM_BACKTRACE/Languages/zh-CN
  ${CMAKE_SOURCE_DIR}/CM_BACKTRACE/Languages/en-US
)

# ====================================================================
# 1. CMSIS Core (system + startup)
# ====================================================================
add_library(cmsis STATIC
  CMSIS/system_gd32f30x.c
  CMSIS/startup_gd32f30x_hd_gcc.s
)
target_include_directories(cmsis PUBLIC ${COMMON_INCLUDES})
target_compile_definitions(cmsis PRIVATE ${DEVICE_DEFINES})

# ====================================================================
# 2. FWLIB — Standard Peripheral Driver Library
# ====================================================================
file(GLOB_RECURSE FWLIB_SOURCES
  FWLIB/Source/*.c
)
add_library(fwlib STATIC ${FWLIB_SOURCES})
target_include_directories(fwlib PUBLIC ${COMMON_INCLUDES})
target_compile_definitions(fwlib PRIVATE ${DEVICE_DEFINES})
target_link_libraries(fwlib PUBLIC cmsis)

# ====================================================================
# 3. Hardware Layer
# ====================================================================
set(HARDWARE_BASE
  HardWare/COMMON/at_cmd_common.c
  HardWare/I2C/i2c.c
)

# Assemble hardware sources based on feature flags
set(HARDWARE_SOURCES ${HARDWARE_BASE})

add_library(hardware STATIC ${HARDWARE_SOURCES})
target_include_directories(hardware PUBLIC ${COMMON_INCLUDES})
target_compile_definitions(hardware PRIVATE ${DEVICE_DEFINES})
target_link_libraries(hardware PUBLIC fwlib)

# ====================================================================
# 4. CM Backtrace (crash analysis)
# ====================================================================
list(APPEND BACKTRACE_SOURCES
  CM_BACKTRACE/cm_backtrace.c
  CM_BACKTRACE/fault_handler/gcc/cmb_fault.S   # GCC version!
)
add_library(cmbacktrace STATIC ${BACKTRACE_SOURCES})
target_include_directories(cmbacktrace PUBLIC ${COMMON_INCLUDES})
target_compile_definitions(cmbacktrace PRIVATE ${DEVICE_DEFINES})
target_link_libraries(cmbacktrace PUBLIC cmsis)

# ====================================================================
# 5. User Application
# ====================================================================
set(USER_SOURCES
  User/main.c
  User/gd32f30x_it.c
  User/systick.c
  User/flash.c
  User/syscalls.c
)

add_executable(${PROJECT_NAME} ${USER_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${DEVICE_DEFINES})

# ---- Link options ----
target_link_options(${PROJECT_NAME} PRIVATE
  -T${CMAKE_SOURCE_DIR}/GD32F303CC.ld
  -Wl,-Map=${PROJECT_NAME}.map
)

# ---- Link libraries ----
target_link_libraries(${PROJECT_NAME}
  cmsis
  fwlib
  hardware
  cmbacktrace
)

# ---- Post-build: generate .hex and .bin ----
if(ENABLE_HEX OR ENABLE_BIN)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Firmware size:"
  )
endif()

if(ENABLE_HEX)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMENT "Generating ${PROJECT_NAME}.hex"
  )
endif()

if(ENABLE_BIN)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin"
  )
endif()

# ---- Print build info ----
message(STATUS "")
message(STATUS "=========================================================")
message(STATUS "  GD32F303CCT6 Project Configuration")
message(STATUS "  MCU       : GD32F303CC (Cortex-M4F, 120 MHz)")
message(STATUS "  Flash     : 256 KB  |  RAM  : 48 KB")
message(STATUS "=========================================================")
message(STATUS "")
