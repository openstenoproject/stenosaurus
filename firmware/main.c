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
// This file is the main entry point for the stenosaurus firmware.

#include "../common/user_button.h"
#include "clock.h"
#include "debug.h"
#include "protocol.h"
#include "sdio.h"
#include "stroke.h"
#include "txbolt.h"
#include "usb.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencmsis/core_cm3.h>
#include "keyboard.h"

#include "../common/leds.h"

int main(void) {
    clock_init();

    setup_user_button();

    setup_leds();

    usb_init(packet_handler);
    //sdio_init();

    //bool card_initialized = false;

    bool pressed = false;

    while (true) {
        if (is_user_button_pressed()) {
            if (!pressed) {
                led_toggle(0);
                usb_keyboard_key_down(KEY_Q);
                usb_keyboard_key_down(KEY_W);
                pressed = true;                
            }
        } else {
            if (pressed) {
                usb_keyboard_key_up(KEY_Q);
                usb_keyboard_key_up(KEY_W);
                led_toggle(0);
                pressed = false;                
            }
        }
        usb_send_keys_if_changed();

#if 0
        if (sdio_card_present() && !card_initialized) {
            print("Card detected.\r\n");

            if (!sdio_card_init()) {
                print("Card not initialized.\r\n\r\n");
                continue;
            }

            print("Initialized card.\r\n\r\n");

            card_initialized = true;
        }
        if (!sdio_card_present() && card_initialized) {
            print("Card removed.\r\n\r\n");
            card_initialized = false;
        }
#endif

#if 0
        if (is_user_button_pressed()) {
            packet txbolt_packet;
            uint32_t stroke = string_to_stroke("PHRO*FR");
            make_packet(stroke, &txbolt_packet);
            serial_usb_send_data(&txbolt_packet.byte[0], txbolt_packet.length);
        }
#endif
    }
}
