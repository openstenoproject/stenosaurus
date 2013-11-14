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
*/

#ifndef STENOSAURUS_STROKE_H
#define STENOSAURUS_STROKE_H

#include <inttypes.h>

static const uint8_t INITIAL_S_BIT = (22);
static const uint8_t INITIAL_T_BIT = (21);
static const uint8_t INITIAL_K_BIT = (20);
static const uint8_t INITIAL_P_BIT = (19);
static const uint8_t INITIAL_W_BIT = (18);
static const uint8_t INITIAL_H_BIT = (17);
static const uint8_t INITIAL_R_BIT = (16);
static const uint8_t A_BIT = (15);
static const uint8_t O_BIT = (14);
static const uint8_t STAR_BIT = (13);
static const uint8_t E_BIT = (12);
static const uint8_t U_BIT = (11);
static const uint8_t FINAL_F_BIT = (10);
static const uint8_t FINAL_R_BIT = (9);
static const uint8_t FINAL_P_BIT = (8);
static const uint8_t FINAL_B_BIT = (7);
static const uint8_t FINAL_L_BIT = (6);
static const uint8_t FINAL_G_BIT = (5);
static const uint8_t FINAL_T_BIT = (4);
static const uint8_t FINAL_S_BIT = (3);
static const uint8_t FINAL_D_BIT = (2);
static const uint8_t FINAL_Z_BIT = (1);
static const uint8_t HASH_BIT = (0);

#define INITIAL_S_STROKE (1 << INITIAL_S_BIT)
#define INITIAL_T_STROKE (1 << INITIAL_T_BIT)
#define INITIAL_K_STROKE (1 << INITIAL_K_BIT)
#define INITIAL_P_STROKE (1 << INITIAL_P_BIT)
#define INITIAL_W_STROKE (1 << INITIAL_W_BIT)
#define INITIAL_H_STROKE (1 << INITIAL_H_BIT)
#define INITIAL_R_STROKE (1 << INITIAL_R_BIT)
#define A_STROKE (1 << A_BIT)
#define O_STROKE (1 << O_BIT)
#define STAR_STROKE (1 << STAR_BIT)
#define E_STROKE (1 << E_BIT)
#define U_STROKE (1 << U_BIT)
#define FINAL_F_STROKE (1 << FINAL_F_BIT)
#define FINAL_R_STROKE (1 << FINAL_R_BIT)
#define FINAL_P_STROKE (1 << FINAL_P_BIT)
#define FINAL_B_STROKE (1 << FINAL_B_BIT)
#define FINAL_L_STROKE (1 << FINAL_L_BIT)
#define FINAL_G_STROKE (1 << FINAL_G_BIT)
#define FINAL_T_STROKE (1 << FINAL_T_BIT)
#define FINAL_S_STROKE (1 << FINAL_S_BIT)
#define FINAL_D_STROKE (1 << FINAL_D_BIT)
#define FINAL_Z_STROKE (1 << FINAL_Z_BIT)
#define HASH_STROKE (1 << HASH_BIT)

// Getters.
// For convenience, these functions are guaranteed to return 0 for false and 1
// for true.
static inline uint32_t get_initial_s_stroke(uint32_t s) {
    return (s >> INITIAL_S_BIT) & 1;
}
static inline uint32_t get_initial_t_stroke(uint32_t s) {
    return (s >> INITIAL_T_BIT) & 1;
}
static inline uint32_t get_initial_k_stroke(uint32_t s) {
    return (s >> INITIAL_K_BIT) & 1;
}
static inline uint32_t get_initial_p_stroke(uint32_t s) {
    return (s >> INITIAL_P_BIT) & 1;
}
static inline uint32_t get_initial_w_stroke(uint32_t s) {
    return (s >> INITIAL_W_BIT) & 1;
}
static inline uint32_t get_initial_h_stroke(uint32_t s) {
    return (s >> INITIAL_H_BIT) & 1;
}
static inline uint32_t get_initial_r_stroke(uint32_t s) {
    return (s >> INITIAL_R_BIT) & 1;
}
static inline uint32_t get_a_stroke(uint32_t s) {
    return (s >> A_BIT) & 1;
}
static inline uint32_t get_o_stroke(uint32_t s) {
    return (s >> O_BIT) & 1;
}
static inline uint32_t get_star_stroke(uint32_t s) {
    return (s >> STAR_BIT) & 1;
}
static inline uint32_t get_e_stroke(uint32_t s) {
    return (s >> E_BIT) & 1;
}
static inline uint32_t get_u_stroke(uint32_t s) {
    return (s >> U_BIT) & 1;
}
static inline uint32_t get_final_f_stroke(uint32_t s) {
    return (s >> FINAL_F_BIT) & 1;
}
static inline uint32_t get_final_r_stroke(uint32_t s) {
    return (s >> FINAL_R_BIT) & 1;
}
static inline uint32_t get_final_p_stroke(uint32_t s) {
    return (s >> FINAL_P_BIT) & 1;
}
static inline uint32_t get_final_b_stroke(uint32_t s) {
    return (s >> FINAL_B_BIT) & 1;
}
static inline uint32_t get_final_l_stroke(uint32_t s) {
    return (s >> FINAL_L_BIT) & 1;
}
static inline uint32_t get_final_g_stroke(uint32_t s) {
    return (s >> FINAL_G_BIT) & 1;
}
static inline uint32_t get_final_t_stroke(uint32_t s) {
    return (s >> FINAL_T_BIT) & 1;
}
static inline uint32_t get_final_s_stroke(uint32_t s) {
    return (s >> FINAL_S_BIT) & 1;
}
static inline uint32_t get_final_d_stroke(uint32_t s) {
    return (s >> FINAL_D_BIT) & 1;
}
static inline uint32_t get_final_z_stroke(uint32_t s) {
    return (s >> FINAL_Z_BIT) & 1;
}
static inline uint32_t get_hash_stroke(uint32_t s) {
    return (s >> HASH_BIT) & 1;
}

// Setters
static inline void set_initial_s_stroke(uint32_t *s) {
    *s |= INITIAL_S_STROKE;
}
static inline void set_initial_t_stroke(uint32_t *s) {
    *s |= INITIAL_T_STROKE;
}
static inline void set_initial_k_stroke(uint32_t *s) {
    *s |= INITIAL_K_STROKE;
}
static inline void set_initial_p_stroke(uint32_t *s) {
    *s |= INITIAL_P_STROKE;
}
static inline void set_initial_w_stroke(uint32_t *s) {
    *s |= INITIAL_W_STROKE;
}
static inline void set_initial_h_stroke(uint32_t *s) {
    *s |= INITIAL_H_STROKE;
}
static inline void set_initial_r_stroke(uint32_t *s) {
    *s |= INITIAL_R_STROKE;
}
static inline void set_a_stroke(uint32_t *s) {
    *s |= A_STROKE;
}
static inline void set_o_stroke(uint32_t *s) {
    *s |= O_STROKE;
}
static inline void set_star_stroke(uint32_t *s) {
    *s |= STAR_STROKE;
}
static inline void set_e_stroke(uint32_t *s) {
    *s |= E_STROKE;
}
static inline void set_u_stroke(uint32_t *s) {
    *s |= U_STROKE;
}
static inline void set_final_f_stroke(uint32_t *s) {
    *s |= FINAL_F_STROKE;
}
static inline void set_final_r_stroke(uint32_t *s) {
    *s |= FINAL_R_STROKE;
}
static inline void set_final_p_stroke(uint32_t *s) {
    *s |= FINAL_P_STROKE;
}
static inline void set_final_b_stroke(uint32_t *s) {
    *s |= FINAL_B_STROKE;
}
static inline void set_final_l_stroke(uint32_t *s) {
    *s |= FINAL_L_STROKE;
}
static inline void set_final_g_stroke(uint32_t *s) {
    *s |= FINAL_G_STROKE;
}
static inline void set_final_t_stroke(uint32_t *s) {
    *s |= FINAL_T_STROKE;
}
static inline void set_final_s_stroke(uint32_t *s) {
    *s |= FINAL_S_STROKE;
}
static inline void set_final_d_stroke(uint32_t *s) {
    *s |= FINAL_D_STROKE;
}
static inline void set_final_z_stroke(uint32_t *s) {
    *s |= FINAL_Z_STROKE;
}
static inline void set_hash_stroke(uint32_t *s) {
    *s |= HASH_STROKE;
}

// Unsetters
static inline void clear_initial_s_stroke(uint32_t *s) {
    *s &= ~INITIAL_S_STROKE;
}
static inline void clear_initial_t_stroke(uint32_t *s) {
    *s &= ~INITIAL_T_STROKE;
}
static inline void clear_initial_k_stroke(uint32_t *s) {
    *s &= ~INITIAL_K_STROKE;
}
static inline void clear_initial_p_stroke(uint32_t *s) {
    *s &= ~INITIAL_P_STROKE;
}
static inline void clear_initial_w_stroke(uint32_t *s) {
    *s &= ~INITIAL_W_STROKE;
}
static inline void clear_initial_h_stroke(uint32_t *s) {
    *s &= ~INITIAL_H_STROKE;
}
static inline void clear_initial_r_stroke(uint32_t *s) {
    *s &= ~INITIAL_R_STROKE;
}
static inline void clear_a_stroke(uint32_t *s) {
    *s &= ~A_STROKE;
}
static inline void clear_o_stroke(uint32_t *s) {
    *s &= ~O_STROKE;
}
static inline void clear_star_stroke(uint32_t *s) {
    *s &= ~STAR_STROKE;
}
static inline void clear_e_stroke(uint32_t *s) {
    *s &= ~E_STROKE;
}
static inline void clear_u_stroke(uint32_t *s) {
    *s &= ~U_STROKE;
}
static inline void clear_final_f_stroke(uint32_t *s) {
    *s &= ~FINAL_F_STROKE;
}
static inline void clear_final_r_stroke(uint32_t *s) {
    *s &= ~FINAL_R_STROKE;
}
static inline void clear_final_p_stroke(uint32_t *s) {
    *s &= ~FINAL_P_STROKE;
}
static inline void clear_final_b_stroke(uint32_t *s) {
    *s &= ~FINAL_B_STROKE;
}
static inline void clear_final_l_stroke(uint32_t *s) {
    *s &= ~FINAL_L_STROKE;
}
static inline void clear_final_g_stroke(uint32_t *s) {
    *s &= ~FINAL_G_STROKE;
}
static inline void clear_final_t_stroke(uint32_t *s) {
    *s &= ~FINAL_T_STROKE;
}
static inline void clear_final_s_stroke(uint32_t *s) {
    *s &= ~FINAL_S_STROKE;
}
static inline void clear_final_d_stroke(uint32_t *s) {
    *s &= ~FINAL_D_STROKE;
}
static inline void clear_final_z_stroke(uint32_t *s) {
    *s &= ~FINAL_Z_STROKE;
}
static inline void clear_hash_stroke(uint32_t *s) {
    *s &= ~HASH_STROKE;
}

// Convert a string representation of a stroke into the internal representation.
// This is intended for testing and debugging.
uint32_t string_to_stroke(const char* s);

#endif // #ifndef STENOSAURUS_STROKE_H
