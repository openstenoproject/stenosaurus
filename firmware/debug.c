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
// This file implements some debug functionality for the Stenosaurus. See the
// header file for interface documentation to this code.

#include "usb.h"

void print(char* str) {
    int len = 0;
    char* b = str;
    while (*b) {
        b++;
        len++;
    }
    while (usb_send_serial_data(str, len) == 0);
}

static char nibble_to_hex_char(uint8_t b) {
    // TODO: The number and letter parts could be collapesed with a base plus
    // offset.
    switch (b & 0xF) {
    case 0:
        return '0';
    case 1:
        return '1';
    case 2:
        return '2';
    case 3:
        return '3';
    case 4:
        return '4';
    case 5:
        return '5';
    case 6:
        return '6';
    case 7:
        return '7';
    case 8:
        return '8';
    case 9:
        return '9';
    case 10:
        return 'A';
    case 11:
        return 'B';
    case 12:
        return 'C';
    case 13:
        return 'D';
    case 14:
        return 'E';
    default:
        return 'F';
    }
}

void print_word(uint32_t word) {
    char buf[11];
    char *b = buf;
    *b++ = '0';
    *b++ = 'x';
    *b++ = nibble_to_hex_char(word >> (3 * 8 + 4));
    *b++ = nibble_to_hex_char(word >> (3 * 8));
    *b++ = nibble_to_hex_char(word >> (2 * 8 + 4));
    *b++ = nibble_to_hex_char(word >> (2 * 8));
    *b++ = nibble_to_hex_char(word >> (8 + 4));
    *b++ = nibble_to_hex_char(word >> 8);
    *b++ = nibble_to_hex_char(word >> 4);
    *b++ = nibble_to_hex_char(word);
    *b++ = 0;
    print(buf);
}

void print_arg1(char *str, uint32_t word) {
    print(str);
    print_word(word);
    print("\r\n");
}