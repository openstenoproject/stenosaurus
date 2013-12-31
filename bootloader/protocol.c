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
// This file defines the communication protocol of the bootloader.
//
// See the .h file for interface details.

// TODO: use a specific macro to go between addresses and pointer to make code
// more portable.

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/f1/bkp.h>
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

void make_success(uint8_t *packet, uint8_t request) {
    packet[0] = RESPONSE_OK;
    packet[1] = request;
    zero(packet + 2, PACKET_SIZE - 2);
}

void make_error(uint8_t *packet, uint8_t request) {
    packet[0] = RESPONSE_ERROR;
    packet[1] = request;
    zero(packet + 2, PACKET_SIZE - 2);
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

// Must be less than or equal to 61 characters.
static const char *device_info = "Stenosaurus has no info yet.";

bool packet_handler(uint8_t *packet) {
    int action = packet[0];
    
    if (action == REQUEST_INFO) {
        *(packet++) = RESPONSE_OK;
        *(packet++) = REQUEST_INFO;
        const char* info = device_info;
        uint32_t len = 0;
        while (*info) {
            *(packet++) = *(info++);
            ++len;
        }
        zero(packet, PACKET_SIZE - 2 - len);
    } else if (action == REQUEST_ERASE_PROGRAM) {
        if (erase_program()) {
            make_success(packet, action);
        } else {
            make_error(packet, action);
        }
    } else if (action == REQUEST_FLASH_PROGRAM) {
        int num_words = packet[1];
        uint8_t *buf = packet + 2; // action + num_words
        uint32_t address = read_word(buf);
        buf += 4;
        // Make sure that there is no overrun with the number of words specified.
        if ((num_words * 4) > (PACKET_SIZE - 6)) {
            make_error(packet, action);
            return false;
        }
        uint8_t *end = buf + num_words * 4;
        while (buf < end) {
            uint32_t word = read_word(buf);
            buf += 4;
            if (!my_flash_program_word(address, word)) {
                make_error(packet, action);
                return false;
            }
            address += 4;
        }
        make_success(packet, action);
    } else if (action == REQUEST_VERIFY_PROGRAM) {
        crc_reset();
        uint32_t num_words = read_word(packet + 1);
        if (PROGRAM_AREA_BEGIN + num_words * 4 > PROGRAM_AREA_END) {
            make_error(packet, action);
            return false;
        }
        uint32_t crc_result = crc_calculate_block((uint32_t*)PROGRAM_AREA_BEGIN,
                                                  num_words);
        packet[0] = RESPONSE_OK;
        packet[1] = action;
        write_word(packet + 2, crc_result);
        zero(packet + 6, PACKET_SIZE - 6);
    } else if (action == REQUEST_BOOTLOADER) {
        packet[0] = RESPONSE_OK;
        packet[1] = action;
        packet[2] = 1;
        zero(packet + 3, PACKET_SIZE - 3);
    } else if (action == REQUEST_RESET) {
        rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
        pwr_disable_backup_domain_write_protect();

        // An argument of one means that we want to reset into bootloader mode.
        bool bootloader = packet[1] == 1 ? true : false;
        packet[0] = RESPONSE_OK;
        packet[1] = action;
        zero(packet + 2, PACKET_SIZE - 2);

        if (bootloader) {
            BKP_DR1 |= 1;
        }
        else {
            BKP_DR1 &= 0xFFFE;
        }

        return true;
    } else if (action == REQUEST_DEBUG) {
        // Fill in with whatever you like while debugging.
        // By default just returns success.
        make_success(packet, action);
    } else {
        make_error(packet, action);
    }
    
    return false;
}
