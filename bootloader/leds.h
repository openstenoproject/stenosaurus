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
// This file is provides simple and convenient access to the LEDs on the 
// WaveShare Open103R. It purposely does not expose all functionality because 
// it's intended to be easy.
//
// This is not necessarily intended to be a part of the final stenosaurus 
// software.

#ifndef STENOSAURUS_BOOTLOADER_LEDS_H
#define STENOSAURUS_BOOTLOADER_LEDS_H

#include <stdbool.h>

// Setup the LEDs. This must be called before any of the other functions here.
void setup_leds(void);

// For all of the functions below num_led should be 0, 1, 2, or 3. Otherwise, 
// the function silently does nothing.

// Turn on the LED specified by num_led.
void led_on(int num_led);

// Turn off the LED specified by num_led.
void led_off(int num_led);

// Toggle the LED specified by num_led.
void led_toggle(int num_led);

// Returns the current status of the LED specified by num_led, 0 for off and 1 
// for on. If num_led is out of range then returns false.
bool get_led_value(int num_led);

#endif // STENOSAURUS_BOOTLOADER_LEDS_H