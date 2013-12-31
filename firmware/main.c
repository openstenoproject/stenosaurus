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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/crc.h>
#include "../common/user_button.h"
#include "usb.h"
#include "protocol.h"
#include "stroke.h"
#include "txbolt.h"
#include "sdio.h"
#include "clock.h"
#include "debug.h"

int main(void) {
    clock_init();

    setup_user_button();

    init_usb(packet_handler);
    init_sdio();

    uint32_t buffer[128];

    while (true) {
        if (is_user_button_pressed()) {
            uint32_t snapshot = system_millis;
            print_arg1("time: ", snapshot);

            print("Initializing card.\r\n");
            if (!sdio_card_init()) {
                print("Failed to initialize.\r\n");
                continue;
            }

            buffer[0] = snapshot;

            if (!sdio_write_block(0, buffer)) {
                print("Failed to write.\r\n");
                continue;
            }

            buffer[0] = 0;

            if (!sdio_read_block(0, buffer)) {
                print("Failed to read.\r\n");
                continue;
            }
            print_arg1("word read: ", buffer[0]);
        }
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
