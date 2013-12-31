// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library.  If not, see <http://www.gnu.org/licenses/>.
//
// This file defines the interface for the serial transmit, or TX, protocol from
// the Baron Online Transcriptor.
//
// See the .c file for implementation details.

#ifndef STENOSAURUS_FIRMWARE_TXBOLT_H
#define STENOSAURUS_FIRMWARE_TXBOLT_H

#include <inttypes.h>

typedef struct {
    // The number of bytes in this packet.
    uint8_t length;
    // The data for the packet. Only the first length bytes are valid.
    uint8_t byte[5];
} packet;

// Takes in a stroke (as defined in stroke.h) and populates the packet.
void make_packet(uint32_t s, packet *p);

#endif // #ifndef STENOSAURUS_FIRMWARE_TXBOLT_H
