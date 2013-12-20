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
 * This is the linker script used by the bootloader so the rom offset starts at
 * the beginning and the length is reduced to 8K. (NOTE: decide on the final 
 * amount). The rest of the rom is 
 * reserved for the application firmware. Note that the application must be
 * 128 bytes aligned.
 */

/* Define memory regions. */
MEMORY
{
        rom (rx) : ORIGIN = 0x08000000, LENGTH = 8K
        ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f1.ld
