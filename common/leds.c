// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this library.  If not, see <http://www.gnu.org/licenses/>.
//
// See leds.h for interface documentation. See below for implementation details.

#include "leds.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void setup_leds(void) {
    // Enable the clock to General Purpose Input Output port C.
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

    // Set pins 9, 10, 11, and 12 to output mode push/pull.
    // TODO: I don't know the meaning of the output MHZ setting.
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, 
                  GPIO9);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, 
                  GPIO10);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, 
                  GPIO11);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, 
                  GPIO12);
}

void led_on(int num_led) {
    if (num_led < 0 || num_led > 3) return;
    gpio_set(GPIOC, GPIO9 << num_led);
}

void led_off(int num_led) {
    if (num_led < 0 || num_led > 3) return;
    gpio_clear(GPIOC, GPIO9 << num_led);
}

void led_toggle(int num_led) {
    if (num_led < 0 || num_led > 3) return;
    gpio_toggle(GPIOC, GPIO9 << num_led);
}

bool get_led_value(int num_led) {
    if (num_led < 0 || num_led > 3) return false;
    return gpio_get(GPIOC, GPIO9 << num_led) ? true : false;
}
