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
// This file defines some debug functionality for the Stenosaurus firmware.
//
// See the .c file for implementation details.

#ifndef STENOSAURUS_FIRMWARE_DEBUG
#define STENOSAURUS_FIRMWARE_DEBUG

// Print a string to the USB emulated serial port.
void print(char* str);

// Print a 32 bit int as a hex string using print.
void print_word(uint32_t word);

// Print a string and a 32 bit word as above.
void print_arg1(char *str, uint32_t word);

#endif // STENOSAURUS_FIRMWARE_DEBUG
