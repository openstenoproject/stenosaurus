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

#include "txbolt.h"

#include <inttypes.h>
#include "stroke.h"

// In the TX Bolt protocol, there are four sets of keys grouped in
// order from left to right. Each byte represents all the keys that
// were pressed in that set. The first two bits indicate which set this
// byte represents. The next bits are set if the corresponding key was
// pressed for the stroke.
//
// 00XXXXXX 01XXXXXX 10XXXXXX 110XXXXX
//   HWPKTS   UE*OAR   GLBPRF    #ZDST
//
// The protocol uses variable length packets of one, two, three or four
// bytes. Only those bytes for which keys were pressed will be
// transmitted. The bytes arrive in order of the sets so it is clear
// when a new stroke starts. Also, if a key is pressed in an earlier
// set in one stroke and then a key is pressed only in a later set then
// there will be a zero byte to indicate that this is a new stroke. So,
// it is reliable to assume that a stroke ended when a lower set is
// seen. Additionally, if there is no activity then the machine will
// send a zero byte every few seconds.

// In this implementation of the protocol we send a zero byte after each stroke
// regardless of it being necessary. It is believed by the implementor that
// this should be compatible with any software that is prepared to receive 
// this protocol. The benefit is that it makes the protocol implementation 
// stateless and reduces delay issues for the final stroke.

void make_packet(uint32_t s, packet *p) {
    p->length = 0;
    uint8_t page;
    
    // Page 1: 00HWPKTS
    page = get_initial_s_stroke(s) |
           get_initial_t_stroke(s) << 1 |
           get_initial_k_stroke(s) << 2 |
           get_initial_p_stroke(s) << 3 |
           get_initial_w_stroke(s) << 4 |
           get_initial_h_stroke(s) << 5;
    if (page) {
        p->byte[0] = page;
        ++(p->length);
    }

    // Page 2: 01UE*OAR
    page = get_initial_r_stroke(s) |
           get_a_stroke(s) << 1 |
           get_o_stroke(s) << 2 |
           get_star_stroke(s) << 3 |
           get_e_stroke(s) << 4 |
           get_u_stroke(s) << 5;
    if (page) {
        p->byte[(p->length)++] = (page | 0b01000000);
    }
    
    // Page 3: 10GLBPRF
    page = get_final_f_stroke(s) |
           get_final_r_stroke(s) << 1 |
           get_final_p_stroke(s) << 2 |
           get_final_b_stroke(s) << 3 |
           get_final_l_stroke(s) << 4 |
           get_final_g_stroke(s) << 5;
    if (page) {
        p->byte[(p->length)++] = (page | 0b10000000);
    }
    
    // Page 5: 110#ZDST
    page = get_final_t_stroke(s) |
           get_final_s_stroke(s) << 1 |
           get_final_d_stroke(s) << 2 |
           get_final_z_stroke(s) << 3 |
           get_hash_stroke(s);
    if (page) {
        p->byte[(p->length)++] = (page | 0b11000000);
    }
}
