// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this library.  If not, see <http://www.gnu.org/licenses/>.
//
// This file implements clock functionality for the Stenosaurus. See the header
// file for interface documentation to this code.

#include "clock.h"

#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/scb.h>

// Rolls over after ~49 days. If the device is on that long then it will reboot.
volatile uint32_t system_millis;

void clock_init(void) {
    // Set the clock to use the 8Mhz internal high speed (hsi) clock as input
    // and set the output of the PLL at 48Mhz. Since we're using USB we are
    // limited to two clock speeds: 48Mhz or 72Mhz.
    // TODO: The documentation for the chip says that HSE must be used for USB.
    // But in the examples we see HSI used with USB and it also seems to work.
    rcc_clock_setup_in_hsi_out_48mhz();

    // One millisecond is clock rate (48Mhz) divided by a thousand = 48K.
    systick_set_reload(48000);
    systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

// Implementing a function with this name makes it the handler for the systick
// interrupt.
void sys_tick_handler(void) {
    ++system_millis;
    if (system_millis == 0) {
        scb_reset_system();
    }
}
