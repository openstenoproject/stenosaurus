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
// This file is the main entry point for the stenosaurus bootloader. The purpose 
// of the bootloader is to allow updating the firmware of the stenosaurus in a 
// safe way.
// 
// Design goals:
// - Update the stenosaurus firmware using client software on the host.
// - Impossible to brick the device with a firmware update.
// - Do not require any drivers.
// - Normal boot should transfer to the application firmware very quickly.
// - Device update should be triggered by the client software on the host.
// - As an emergency backup there should be a manual way to enter the bootloader
//   into firmware update mode.

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencmsis/core_cm3.h>
#include "usb.h"
#include "user_button.h"
#include "leds.h"
#include "protocol.h"

int main(void) {
    // Set the clock to use the 8Mhz internal high speed (hsi) clock as input 
    // and set the output of the PLL at 48Mhz.
    // TODO: The documentation for the chip says that HSE must be used for USB. 
    // But in the examples we see HSI used with USB and it also seems to work.
    rcc_clock_setup_in_hsi_out_48mhz();
    
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_CRCEN);
    
    setup_user_button();
    setup_leds();
    
    init_usb(packet_handler);
    
    // Tell the chip that when it returns from an interrupt it should go to sleep.
    SCB_SCR |= SCB_SCR_SLEEPONEXIT;
    // Then go to sleep.
    while (true) {
        // In theory this should never return since we set SLEEPONEXIT. However,
        // the documentation states there can be spurious events that wake the
        // device and that this must be handled.
        __WFI();
        // TODO: Consider using WFE + SEVONPEND and call pollusb in a loop. This
        // would be event faster since it avoid interrupts.
    }
    // TODO: Investigate the right way to put the processor to sleep and the 
    // various sleep modes to find out which is the right one here.
}
