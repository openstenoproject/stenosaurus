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
// This file is the main entry point for the stenosaurus firmware.

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/crc.h>
#include "../common/user_button.h"
#include "../common/leds.h"

int main(void) {
    // Set the clock to use the 8Mhz internal high speed (hsi) clock as input 
    // and set the output of the PLL at 48Mhz.
    // TODO: The documentation for the chip says that HSE must be used for USB. 
    // But in the examples we see HSI used with USB and it also seems to work.
    rcc_clock_setup_in_hsi_out_48mhz();

    setup_user_button();
    setup_leds();

    while (true) {
        if (is_user_button_pressed()) {
            led_toggle(2);
        }
    }
}
