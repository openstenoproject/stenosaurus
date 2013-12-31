// This file is part of the libopencm3 project.
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

// TODO: Comment it up.

#include "stroke.h"

#include <inttypes.h>
#include <stdbool.h>

uint32_t string_to_stroke(const char* s) {
    bool initial = true;
    uint32_t stroke = 0;
    while (*s != 0) {
        switch (*s) {
        case 'S':
        case 's':
            stroke |= initial ? INITIAL_S_STROKE : FINAL_S_STROKE;
            break;

        case 'T':
        case 't':
            stroke |= initial ? INITIAL_T_STROKE : FINAL_T_STROKE;
            break;

        case 'K':
        case 'k':
            stroke |= INITIAL_K_STROKE;
            break;

        case 'P':
        case 'p':
            stroke |= initial ? INITIAL_P_STROKE : FINAL_P_STROKE;
            break;

        case 'W':
        case 'w':
            stroke |= INITIAL_W_STROKE;
            break;

        case 'H':
        case 'h':
            stroke |= INITIAL_H_STROKE;
            break;

        case 'R':
        case 'r':
            stroke |= initial ? INITIAL_R_STROKE : FINAL_R_STROKE;
            break;

        case 'A':
        case 'a':
            stroke |= A_STROKE;
            initial = false;
            break;

        case 'O':
        case 'o':
            stroke |= O_STROKE;
            initial = false;
            break;

        case '*':
            stroke |= STAR_STROKE;
            initial = false;
            break;

        case 'E':
        case 'e':
            stroke |= E_STROKE;
            initial = false;
            break;

        case 'U':
        case 'u':
            stroke |= U_STROKE;
            initial = false;
            break;

        case 'F':
        case 'f':
            stroke |= FINAL_F_STROKE;
            break;

        case 'B':
        case 'b':
            stroke |= FINAL_B_STROKE;
            break;

        case 'L':
        case 'l':
            stroke |= FINAL_L_STROKE;
            break;

        case 'G':
        case 'g':
            stroke |= FINAL_G_STROKE;
            break;

        case 'D':
        case 'd':
            stroke |= FINAL_D_STROKE;
            break;

        case 'Z':
        case 'z':
            stroke |= FINAL_Z_STROKE;
            break;

        case '#':
            stroke |= HASH_STROKE;
            break;
        }
        s++;
    }
    return stroke;
}
