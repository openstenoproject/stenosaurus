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
 * This is the linker script used by the actual firmware so the rom offset after
 * the area reserved for the bootloader. 
 */

/* Define memory regions. */
MEMORY
{
        rom (rx) : ORIGIN = 0x08002000, LENGTH = 248K
        ram (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f1.ld
