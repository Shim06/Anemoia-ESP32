#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include <Arduino.h>
#include <stdint.h>

static inline void gpioFastWrite(uint8_t pin, bool level)
{
    if (pin < 32)
    {
        if (level) GPIO.out_w1ts = (1UL << pin);
        else GPIO.out_w1tc = (1UL << pin);
    }
    else
    {
        uint32_t mask = (1UL << (pin - 32));
        if (level) GPIO.out1_w1ts.val = mask;
        else GPIO.out1_w1tc.val = mask;
    }
}

static inline bool gpioFastRead(uint8_t pin)
{
    if (pin < 32) { return (GPIO.in >> pin) & 0x1; }
    else { return (GPIO.in1.val >> (pin - 32)) & 0x1; }
}