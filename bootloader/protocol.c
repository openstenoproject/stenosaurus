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
// This file defines the communication protocol of the bootloader.
//
// See the .h file for interface details.

// TODO: use a specific macro to go between addresses and pointer to make code
// more portable.

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"
#include "../common/leds.h"
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/crc.h>
#include "memorymap.h"

static bool erase_program(void) {
    flash_unlock();
    for (uint32_t i = PROGRAM_AREA_BEGIN; i<= PROGRAM_AREA_END; 
         i += PROGRAM_PAGE_SIZE) {
        flash_erase_page(i);
    }
    uint32_t *buf = (uint32_t*)PROGRAM_AREA_BEGIN;
    uint32_t *end = (uint32_t*)PROGRAM_AREA_END;
    while (buf != end) {
        if (*buf++ != 0xFFFFFFFF) {
            return false;
        }
    }
    return true;
}

static bool my_flash_program_word(uint32_t address, uint32_t word) {
    address += PROGRAM_AREA_BEGIN;
    if (address >= PROGRAM_AREA_END - 4) return false;
    flash_program_word(address, word);
    return (*(uint32_t*)address) == word;
}

static const uint8_t PACKET_SIZE = 64;

static const int REQUEST_INFO = 1;
static const int REQUEST_ERASE_PROGRAM = 2;
static const int REQUEST_FLASH_PROGRAM = 3;
static const int REQUEST_VERIFY_PROGRAM = 4;
static const int REQUEST_BOOTLOADER = 5;
static const int REQUEST_RESET = 6;
static const int REQUEST_DEBUG = 9;

static const uint8_t RESPONSE_OK = 1;
static const uint8_t RESPONSE_ERROR = 2;

void fill(uint8_t *buf, uint8_t size, uint8_t value) {
    uint8_t *end = buf + size;
    while (buf != end) *buf++ = value;
}

void zero(uint8_t *buf, uint8_t size) {
    fill(buf, size, 0);
}

void make_success(uint8_t *packet) {
    packet[0] = RESPONSE_OK;
    zero(packet + 1, PACKET_SIZE - 1);
}

void make_error(uint8_t *packet) {
    packet[0] = RESPONSE_ERROR;
    zero(packet + 1, PACKET_SIZE - 1);
}

uint32_t read_word(uint8_t *b) {
    return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
}

void write_word(uint8_t *packet, uint32_t word) {
    packet[0] = word & 0xFF;
    packet[1] = (word >> 8) & 0xFF;
    packet[2] = (word >> 16) & 0xFF;
    packet[3] = (word >> 24) & 0xFF;
}

// Must be less than 62 characters.
static const char *device_info = "Hi";

bool packet_handler(uint8_t *packet, uint8_t size) {
    (void)size;

    int action = packet[0];
    
    if (action == REQUEST_INFO) {
        *(packet++) = RESPONSE_OK;
        const char* info = device_info;
        while (*info) {
            *(packet++) = *(info++);
        }
        *packet = 0;
    } else if (action == REQUEST_ERASE_PROGRAM) {
        if (erase_program()) {
            make_success(packet);
        } else {
            make_error(packet);
        }
    } else if (action == REQUEST_FLASH_PROGRAM) {
        int num_words = packet[1];
        uint8_t *buf = packet + 2; // action + num_words
        uint32_t address = read_word(buf);
        buf += 4;
        // Make sure that there is no overrun with the number of words specified.
        if ((num_words * 4) > (PACKET_SIZE - 6)) {
            make_error(packet);
            return false;
        }
        uint8_t *end = buf + num_words * 4;
        while (buf < end) {
            uint32_t word = read_word(buf);
            buf += 4;
            if (!my_flash_program_word(address, word)) {
                make_error(packet);
                return false;
            }
            led_toggle(3);
            address += 4;
        }
        make_success(packet);
    } else if (action == REQUEST_VERIFY_PROGRAM) {
        crc_reset();
        uint32_t num_words = read_word(packet + 1);
        if (PROGRAM_AREA_BEGIN + num_words * 4 > PROGRAM_AREA_END) {
            make_error(packet);
            return false;
        }
        uint32_t crc_result = crc_calculate_block((uint32_t*)PROGRAM_AREA_BEGIN,
                                                  num_words);
        packet[0] = RESPONSE_OK;
        write_word(packet + 1, crc_result);
        zero(packet + 5, PACKET_SIZE - 5);
    } else if (action == REQUEST_BOOTLOADER) {
        packet[0] = RESPONSE_OK;
        packet[1] = 1;
    } else if (action == REQUEST_RESET) {
        led_toggle(3);
        make_success(packet);
        return true;
    } else if (action == REQUEST_DEBUG) {
        // find last non-erased word
        uint32_t *end = (uint32_t*)PROGRAM_AREA_BEGIN;
        uint32_t *buf = (uint32_t*)PROGRAM_AREA_END;
        while (buf >= end) {
            if (*buf != 0xFFFFFFFF) {
                break;
            }
            --buf;
        }
        if (buf >= end) {
            packet[0] = RESPONSE_OK;
            write_word(packet + 1, (uint32_t)buf);
            write_word(packet + 5, *buf);
            zero(packet + 9, PACKET_SIZE - 9);
        } else {
            make_error(packet);
            return false;
        }
        
        for (int i = 0; i < 4; ++i) {
            if (packet[1] & (1 << i)) {
                led_toggle(i);
            }
        }
    } else {
        make_error(packet);
    }
    
    return false;
}
