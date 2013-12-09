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
// This file is provides convenient access to the USER button on the WaveShare 
// Open103R. 
//
// This is not necessarily intended to be a part of the final stenosaurus 
// software.

#ifndef STENOSAURUS_BOOTLOADER_USER_BUTTON_H
#define STENOSAURUS_BOOTLOADER_USER_BUTTON_H

#include <stdbool.h>

// Sets up the user button. Must be called before using any of the other
// functions here.
void setup_user_button(void);

// Get the raw value of the button input. Input is not debounced.
bool is_user_button_down(void);

// This function debounces the button presses so there are no suprious on/off 
// events but only works if the function is called constantly in a loop.
bool is_user_button_pressed(void);

#endif // STENOSAURUS_BOOTLOADER_USER_BUTTON_H