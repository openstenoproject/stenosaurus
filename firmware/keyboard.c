// This file is part of the stenosaurus project.
//
// Copyright (C) 2014 Hesky Fisher <hesky.fisher@gmail.com>
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
// This file implements keyboard related functionality for the Stenosaurus. See
// the header file for interface documentation to this code.

#include "keyboard.h"

#include <stdint.h>


/*
#define FIFO_SIZE 256
static struct {
	uint32_t front;
	uint32_t count;
	keyboard_event data[FIFO_SIZE];
} key_fifo;
*/

/*
void keyboard_add_event(keyboard_event event) {
	uint32_t write_index = (key_fifo.front + key_fifo.count) % FIFO_SIZE;
	key_fifo[write_index] = event;
	// If we reached the end then throw away the oldest element.
	if (count == FIFO_SIZE) {

	}
	// If we've circled around then drop the oldest entry.
	if (write_index == front) {
		front = ()
	}
	key_fifo[write] = event;
	write = (write + 1) % FIFO_SIZE;
	if (write > BUFFER_SIZE)
}
*/
