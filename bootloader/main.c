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
#include <libopencm3/stm32/crc.h>
#include "usb.h"
#include "../common/user_button.h"
#include "../common/leds.h"
#include "protocol.h"
#include "memorymap.h"
#include <libopencm3/stm32/f1/bkp.h>

__attribute__ ((noreturn))
static void run_firmware(void) {
    const uint32_t * const FIRMWARE_BASE = (const uint32_t *)PROGRAM_AREA_BEGIN;
    // Set the address of the NVIC to the firmware.
    SCB_VTOR = PROGRAM_AREA_BEGIN;
    // Set the stack pointer for the firmware and branch to the firmware's
    // reset handler.
    asm volatile("msr msp, %0\n\t"
                 "bx %1"
                 : : "r" (FIRMWARE_BASE[0]), "r" (FIRMWARE_BASE[1]) : );
    // Convince the compiler that this function will not return.
    for(;;);
}

static bool firmware_is_valid(void) {
    const uint32_t * const FIRMWARE_BASE = (const uint32_t *)PROGRAM_AREA_BEGIN;
    
    if (FIRMWARE_BASE[1] < PROGRAM_AREA_BEGIN) {
        return false;
    }
    if (FIRMWARE_BASE[1] >= PROGRAM_AREA_END) {
        return false;
    }
    // The value of erased flash is all ones (0xFFFFFFFF). After we write the 
    // firmware we write the length of the program in 32 bit words and the crc 
    // followed by a zero. We use that to verify the program before running it. 
    // We search backwards to find the zero and read the two other words.
    uint32_t *end = ((uint32_t*)PROGRAM_AREA_BEGIN) + 2;
    uint32_t *buf = (uint32_t*)PROGRAM_AREA_END;
    while (buf >= end) {
        if (*buf == 0) {
            break;
        }
        --buf;
    }
    if (buf < end) return false;
    
    buf -= 2;
    uint32_t program_length = buf[0];
    uint32_t program_crc = buf[1];
    // TODO: Send a pull request to make the argument const.
    uint32_t crc_result = crc_calculate_block((uint32_t*)FIRMWARE_BASE, program_length);
    if (program_crc != crc_result) return false;
    
    return true;
}

static bool should_run_firmware(void) {
    // By default we should run the firmware, unless:
    // - The USER button is pressed.
    if (is_user_button_down()) {
        return false;
    }
    // - The system was reset specifically to run the bootloader.
    // We use bit 1 of backup data register 1 to indicate that the bootloader 
    // should run.
    if (BKP_DR1 & 1) {
        // Reset the bit so we don't come back into the bootloader next time.
        BKP_DR1 &= 0xFFFE;
        return false;
    }
    // The firmware program is invalid.
    if (!firmware_is_valid()) {
        return false;
    }

    return true;
}

int main(void) {
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_CRCEN);
    // TODO: Check if we also need RCC_APB1ENR_PWREN.
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_BKPEN);
    
    // TODO: these need to be torn down too when launching the firmware.
    setup_user_button();
    setup_leds();
    
    if (should_run_firmware()) {
        led_toggle(0);
        rcc_peripheral_disable_clock(&RCC_AHBENR, RCC_AHBENR_CRCEN);
        // TODO: Check if we also need RCC_APB1ENR_PWREN.
        rcc_peripheral_disable_clock(&RCC_APB1ENR, RCC_APB1ENR_BKPEN);
        run_firmware();
    }
    led_toggle(1);

    // Set the clock to use the 8Mhz internal high speed (hsi) clock as input 
    // and set the output of the PLL at 48Mhz.
    // TODO: The documentation for the chip says that HSE must be used for USB. 
    // But in the examples we see HSI used with USB and it also seems to work.
    rcc_clock_setup_in_hsi_out_48mhz();

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
