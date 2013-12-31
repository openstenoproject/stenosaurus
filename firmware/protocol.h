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
// This file defines the communication protocol of the firmware.
//
// See the .c file for implementation details.

#ifndef STENOSAURUS_FIRMWARE_PROTOCOL_H
#define STENOSAURUS_FIRMWARE_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

// This function is called when there is a packet from the host. It will do
// whatever is requested and will place the response in the same buffer. The
// buffer must be at least 64 bytes long.
bool packet_handler(uint8_t *packet);

#endif // STENOSAURUS_FIRMWARE_PROTOCOL_H