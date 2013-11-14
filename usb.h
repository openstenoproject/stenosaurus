/*
* This file is part of the stenosaurus project.
*
* Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
*
* This library is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* This file captures the USB interface for the Stenosaurus.
*/

#include <inttypes.h>

// Initialize usb.
void init_usb(void);

// Perform any usb operations necessary right now.
void poll_usb(void);

// Send some data via the USB serial link
void serial_usb_send_data(void *buf, int len);