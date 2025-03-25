# ## wolfSSL/wolfCrypt library
file(GLOB WOLFSSL_SRC
    "${WOLFSSL_ROOT}/src/*.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/*.c"
)

file(GLOB WOLFSSL_EXCLUDE
    "${WOLFSSL_ROOT}/src/bio.c"
    "${WOLFSSL_ROOT}/src/conf.c"
    "${WOLFSSL_ROOT}/src/pk.c"
    "${WOLFSSL_ROOT}/src/x509.c"
    "${WOLFSSL_ROOT}/src/ssl_*.c"
    "${WOLFSSL_ROOT}/src/x509_*.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/misc.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/evp.c"
)

foreach(WOLFSSL_EXCLUDE ${WOLFSSL_EXCLUDE})
    list(REMOVE_ITEM WOLFSSL_SRC ${WOLFSSL_EXCLUDE})
endforeach()

include_directories(${WOLFSSL_ROOT})

# Include lwIP
message("$ENV{PICO_SDK_PATH}/lib/lwip/include")
add_library(wolfssl STATIC
    ${WOLFSSL_SRC}
)

target_include_directories(wolfssl PRIVATE
    ${WOLFSSL_CONFIG}
    $ENV{PICO_SDK_PATH}/lib/lwip/src/include
    $ENV{PICO_SDK_PATH}/src/rp2_common/pico_lwip/include/
    $ENV{PICO_SDK_PATH}/src/rp2_common/pico_rand/include/
)

target_compile_definitions(wolfssl PUBLIC
    WOLFSSL_USER_SETTINGS
)

target_link_libraries(wolfssl pico_stdlib )
### End of wolfSSL/wolfCrypt library