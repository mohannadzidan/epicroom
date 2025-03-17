# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(epicroom C CXX ASM)

set(LOGGING_LEVEL 5)
include(secrets.cmake)
# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

file(GLOB sourceFiles
     "src/*.cpp"
     "src/*.c"
)
file(GLOB headerFiles
     "include/*.h"
)

include_directories(include)

add_executable(epicroom_background ${sourceFiles} )
target_compile_definitions(epicroom_background PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        LOGGING_LEVEL=${LOGGING_LEVEL}
        )
target_include_directories(epicroom_background PRIVATE
        headerFiles
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(epicroom_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

pico_enable_stdio_usb(epicroom_background 1)
pico_add_extra_outputs(epicroom_background)


add_executable(epicroom_poll  ${sourceFiles})

target_compile_definitions(epicroom_poll PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        LOGGING_LEVEL=${LOGGING_LEVEL}
        )
target_include_directories(epicroom_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(epicroom_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )


pico_enable_stdio_usb(epicroom_poll 1)
pico_add_extra_outputs(epicroom_poll)
