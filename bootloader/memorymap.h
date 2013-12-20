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
// This file defines the layout of flash in the chip.

#ifndef STENOSAURUS_BOOTLOADER_MEMORYMAP_H
#define STENOSAURUS_BOOTLOADER_MEMORYMAP_H

// The area where the firmware program resides.
static const uint32_t PROGRAM_AREA_BEGIN = 0x08000000 + 1024 * 8;
// The size of pages in flash.
static const uint32_t PROGRAM_PAGE_SIZE = 1024 * 2;
// The end of the area where the firmware program resides. This address is not
// valid to read or write.
static const uint32_t PROGRAM_AREA_END = 0x08000000 + 1024 * 256;

#endif // STENOSAURUS_BOOTLOADER_MEMORYMAP_H
