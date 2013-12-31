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
// See user_button.h for interface documentation. See below for implementation
// details.

#include "user_button.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void setup_user_button(void) {
    // Enable clock to General Purpose Input Output port A (GPIOA).
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    // Set pin 9 of GPIOA as to be used for input and to have a pull up or 
    // pull down resistor.
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9);
    // Set the output register for GPIOA pin9, which when configured as 
    // input with pullup/down, enables the pull-up resistor (resetting the 
    // pin, which is the default, enables the pull down resistor instead). 
    // This means that a button press will be indicated by a low value on 
    // the GPIOA pin 9. This is because the button on the WaveShare board is 
    // wired to connect the pin to ground when pressed. If it were wired to 
    // Vdd then we would have used a pull down resistor instead.
    gpio_set(GPIOA, GPIO9);
}

bool is_user_button_down(void) {
    return (GPIOA_IDR & GPIO9) == 0;
}

static const int DEBOUNCE_TICKS = 20000;
static int debounce = 0;

bool is_user_button_pressed(void) {
    bool pressed = (GPIOA_IDR & GPIO9) == 0;
    if (debounce == 0 && pressed) {
        debounce += 1;
        return true;
    } else if (debounce > 0 && debounce < DEBOUNCE_TICKS) {
        ++debounce;
    } else if (!pressed && debounce == DEBOUNCE_TICKS) {
        ++debounce;
    } else if (debounce > DEBOUNCE_TICKS && debounce < 2*DEBOUNCE_TICKS) {
        ++debounce;
    } else if (!pressed && debounce == 2*DEBOUNCE_TICKS) {
        debounce = 0;
    }
    return false;
}
