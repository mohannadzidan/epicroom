#include "pico/stdlib.h"
#include "hardware.h"
#include "log.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

uint8_t __state = 0;

void init()
{
#ifdef PICO_DEFAULT_LED_PIN
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

void ledOn()
{
    ledPut(1);
}

void ledOff()
{
    ledPut(0);
}

void ledToggle()
{
    ledPut(!__state);
}

void ledPut(u8_t state)
{
    __state = state;
    // log_t("Led is %s", __state ? "on" : "off");
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, __state);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, __state);
#endif
}