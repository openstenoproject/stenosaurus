/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "stroke.h"
#include "txbolt.h"
#include "usb.h"

static void clock_setup(void) {
    // Set the clock to use the 8Mhz internal high speed (hsi) clock as input 
    // and set the output of the PLL at 48Mhz.
    rcc_clock_setup_in_hsi_out_48mhz();
}

static void led_setup(void)
{
        /* Enable GPIOC clock. */
        rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

        /* Set GPIO9 (in GPIO port C) to 'output push-pull'. */
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, GPIO9);
} 

static void button_setup(void) {
        /* Enable GPIOA clock. */
        rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

        /* Set GPIO9 (in GPIO port A) to 'input with pull-up resistor'. */
        gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                      GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9);
        gpio_set(GPIOA, GPIO9);
}

const int DEBOUNCE_TICKS = 20000;
static int debounce = 0;
static bool is_button_pressed(void) {
    bool pressed = (GPIOA_IDR & (1 << 9)) == 0;
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

static void toggle_led(void) {
  gpio_toggle(GPIOC, GPIO9);
}

int main(void) {
    clock_setup();
  
    // Modify the settings for the high speed Advanced Peripheral Bus (APB2) so 
    // that it is used for an "alternate function". It's not clear to me why. I
    // thought the USB was on APB1.
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);
    // Turn off JTAG but keep SWD on. I don't know why. I can only assume that
    // some of the pins are shared so we need to turn off the function we don't
    // want.
    AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

    led_setup();
    button_setup();
    init_usb();
    
    packet txbolt_packet;
    
    while (1) {
        if (is_button_pressed()) {
            toggle_led();
            uint32_t stroke = string_to_stroke("PHRO*FR");
            make_packet(stroke, &txbolt_packet);
            serial_usb_send_data(&txbolt_packet.byte[0], txbolt_packet.length);
            toggle_led();
        }
        
        poll_usb();
    }
}