// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
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
// This file implements the host application to work with the Stenosaurus.

#include <hidapi/hidapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <wchar.h>

// Headers needed for sleeping.
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void sleep(int miliseconds) {
#ifdef WIN32
    Sleep(miliseconds);
#else
    usleep(miliseconds *1000);
#endif
}

#define UNUSED(x) (void)(x)

static const int STENOSAURUS_VID = 0x6666;
static const int STENOSAURUS_PID = 1;

static const uint8_t PACKET_SIZE = 64;

static const int REQUEST_INFO = 1;
static const int REQUEST_ERASE_PROGRAM = 2;
static const int REQUEST_FLASH_PROGRAM = 3;
static const int REQUEST_VERIFY_PROGRAM = 4;
static const int REQUEST_BOOTLOADER = 5;
static const int REQUEST_RESET = 6;
static const int REQUEST_DEBUG = 9;

static const uint8_t RESPONSE_UNSOLICITED = 0;
static const uint8_t RESPONSE_OK = 1;
static const uint8_t RESPONSE_ERROR = 2;

void mypause(void) {
    printf("Press enter to continue ");
    char junk[2];
    fgets(junk, 2, stdin);
}

void write_word(uint8_t *packet, uint32_t word) {
    packet[0] = word & 0xFF;
    packet[1] = (word >> 8) & 0xFF;
    packet[2] = (word >> 16) & 0xFF;
    packet[3] = (word >> 24) & 0xFF;
}

void make_erase_packet(uint8_t *packet) {
    packet[0] = REQUEST_ERASE_PROGRAM;
    memset(packet + 1, 0, PACKET_SIZE - 1);
}

void make_verify_packet(uint8_t *packet, uint32_t program_size) {
    packet[0] = REQUEST_VERIFY_PROGRAM;
    write_word(packet + 1, program_size);
    memset(packet + 5, 0, PACKET_SIZE - 5);
}

void make_debug_packet(uint8_t *packet, uint32_t param) {
    packet[0] = REQUEST_DEBUG;
    write_word(packet + 1, param);
    memset(packet + 5, 0, PACKET_SIZE - 5);
}

void make_bootloader_packet(uint8_t* packet) {
    packet[0] = REQUEST_BOOTLOADER;
    memset(packet + 1, 0, PACKET_SIZE - 1);
}

void make_reset_packet(uint8_t* packet, bool bootloader) {
    packet[0] = REQUEST_RESET;
    packet[1] = (bootloader ? 1 : 0);
    memset(packet + 2, 0, PACKET_SIZE - 2);
}

bool send_receive(hid_device *handle, unsigned char * const packet) {
    uint8_t buf[PACKET_SIZE + 1];
    buf[0] = 0;
    memcpy(buf + 1, packet, PACKET_SIZE);

    int res = hid_write(handle, buf, PACKET_SIZE + 1);
    if (res < 0) {
        printf("Failed to send.\n");
        return false;
    }
    res = hid_read_timeout(handle, packet, PACKET_SIZE,  30 * 1000);
    //res = hid_read(handle, packet, PACKET_SIZE);
    if (res <= 0) {
        printf("failed to receive.\n");
        return false;
    }

    bool result = false;

    if (packet[0] == 1) {
        result = true;
    } else if (packet[0] == 2) {
        result = false;
    } else {
        printf("Unknown response\n");
        result = false;
    }

    return result;
}

bool is_bootloader(hid_device *handle, bool *result) {
    uint8_t packet[PACKET_SIZE];
    make_bootloader_packet(packet);
    if (send_receive(handle, packet)) {
        *result = (packet[2] == 1) ? true : false;
        return true;
    }
    return false;
}

bool send_reset(hid_device *handle, bool bootloader) {
    uint8_t packet[PACKET_SIZE];
    make_reset_packet(packet, bootloader);
    return send_receive(handle, packet);
}

bool connect(hid_device** handle) {
    *handle = hid_open(STENOSAURUS_VID, STENOSAURUS_PID, NULL);
    return *handle != 0;
}

hid_device* enter_device_mode(bool bootloader) {
    int attempts = 5;
    hid_device *handle = 0;
    while (true) {
        if (!connect(&handle)) {
            printf("Could not find device.\n");
            if (--attempts == 0) return 0;
            sleep(1000);
        } else {
            bool result;
            if (!is_bootloader(handle, &result)) {
                printf("Could not communicate with device.\n");
                if (--attempts == 0) return 0;
                sleep(1000);
            } else if (result == bootloader) {
                if (bootloader) {
                    printf("In bootloader mode.\n");
                } else {
                    printf("In application mode.\n");
                }
                return handle;
            } else {
                if (bootloader) {
                    printf("Not in bootloader mode.\n");
                } else {
                    printf("Not in application mode.\n");
                }
                if (--attempts == 0) {
                    hid_close(handle);
                    return 0;
                }
                send_reset(handle, bootloader);
                hid_close(handle);
                printf("Switching to requested mode.\n");
                sleep(2000);
            }
        }
    }
}

hid_device* enter_bootloader() {
    return enter_device_mode(true);
}

// This algorithm is described by the Rocksoft^TM Model CRC Algorithm as follows:
// Name : "CRC-32"
// Width : 32
// Poly : 04C11DB7
// Init : FFFFFFFF
// RefIn : True
// RefOut : True
// XorOut : FFFFFFFF
// Check : CBF43926

static const uint32_t CRC_TABLE[16] = { // Nibble lookup table for 0x04C11DB7 polynomial
    0x00000000,0x04C11DB7,0x09823B6E,0x0D4326D9,0x130476DC,0x17C56B6B,0x1A864DB2,0x1E475005,
    0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61,0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD
};

uint32_t compute_crc(const uint8_t * buf, uint32_t size) {
    uint32_t result = 0xFFFFFFFF;
    size = size >> 2; // /4

    while(size--) {
        // TODO: Fix this to not assume punning is legal.
        result = result ^ *((uint32_t *)buf); // Apply all 32-bits
        buf += 4;

        // Process 32-bits, 4 at a time, or 8 rounds
        result = (result << 4) ^ CRC_TABLE[result >> 28]; // Assumes 32-bit reg, masking index to 4-bits
        result = (result << 4) ^ CRC_TABLE[result >> 28]; //  0x04C11DB7 Polynomial used in STM32
        result = (result << 4) ^ CRC_TABLE[result >> 28];
        result = (result << 4) ^ CRC_TABLE[result >> 28];
        result = (result << 4) ^ CRC_TABLE[result >> 28];
        result = (result << 4) ^ CRC_TABLE[result >> 28];
        result = (result << 4) ^ CRC_TABLE[result >> 28];
        result = (result << 4) ^ CRC_TABLE[result >> 28];
    }

    return(result);
}

bool flash_program(const char  * const filename) {
    int res;
    hid_device *handle;
    uint8_t *b, *p;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file: %s\n", filename);
        return false;
    }

#define PROGRAM_MEMORY_SIZE ((256 - 8) * 1024)
#define MAX_PROGRAM_SIZE (PROGRAM_MEMORY_SIZE - 3 * 4)

    uint8_t program_buffer[PROGRAM_MEMORY_SIZE];
    memset(program_buffer, 0xFF, PROGRAM_MEMORY_SIZE);
    size_t bytes_read = fread(program_buffer, 1, MAX_PROGRAM_SIZE, fp);

    if (!feof(fp)) {
        printf("File is bigger than max program size (%u): %s\n", MAX_PROGRAM_SIZE, filename);
        fclose(fp);
        return false;
    }
    fclose(fp);

    uint32_t padding_bytes = (4 - (bytes_read % 4)) % 4;

    uint32_t program_length = (bytes_read + padding_bytes) / 4;
    uint32_t program_crc = compute_crc(program_buffer, program_length * 4);

    b = program_buffer + bytes_read + padding_bytes;

    write_word(b, program_length);
    write_word(b + 4, program_crc);
    write_word(b + 8, 0);

    uint32_t words_to_write = program_length + 3;
    uint32_t full_crc = compute_crc(program_buffer, PROGRAM_MEMORY_SIZE);

#undef MAX_PROGRAM_SIZE

    uint8_t packet[PACKET_SIZE];

    // Sequence:
    // Get info to make sure we're talking to the right thing.?
    // Send bootloader request. A one means we are in the bootloader. A zero means we are not.
    // If we are not in bootloader then reser to bootloader mode and try again.
    // Send erase.
    // send many flash instructions to populate the program
    // call verify on the whole flash
    // reset
    // if there are any errors, report error, try again?

    // Get into bootloader mode.
    handle = enter_bootloader();
    if (handle == 0) {
        printf("Could not enter bootloader mode.\n");
        return false;
    }

    // Erase current program.
    make_erase_packet(packet);
    if (!send_receive(handle, packet)) {
        printf("Could not erase program.\n");
        return false;
    }

    // Flash program
    uint32_t address = 0;
    b = program_buffer;
    while (words_to_write > 0) {
        memset(packet, 0, PACKET_SIZE);
        packet[0] = REQUEST_FLASH_PROGRAM;

        write_word(packet + 2, address);

        int words_written = 0;
        int words_left = (PACKET_SIZE - 6) / 4;

        p = packet + 6;

        while (words_to_write != 0 && words_left != 0) {
            *p++ = *b++;
            *p++ = *b++;
            *p++ = *b++;
            *p++ = *b++;

            --words_left;
            --words_to_write;
            ++words_written;
            address += 4;
        }

        packet[1] = words_written;

        if (!send_receive(handle, packet)) {
            printf("Could not flash program at address %u\n", address - words_written);
            return false;
        }
    }

    // verify
    // TODO: There shouldn't be a need for an argument to this function.
    make_verify_packet(packet, PROGRAM_MEMORY_SIZE / 4);
    if (!send_receive(handle, packet)) {
        printf("Failed to send verify request.\n");
        return false;
    }
    uint32_t received_crc = packet[2] | (packet[3] << 8) | (packet[4] << 16) | (packet[5] << 24);
    if (received_crc != full_crc) {
        printf("CRC mismatch. Actual: %u, Received: %u\n", full_crc, received_crc);
        return false;
    }

    // reset
    make_reset_packet(packet, false);
    if (!send_receive(handle, packet)) {
        // Hmm... failure to reset shouldn't necessarily be a failure to flash.
        printf("Could not reset.\n");
        return false;
    }

    return true;

#undef PROGRAM_MEMORY_SIZE
}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    int res;

    int result = -1;

    if (hid_init()) {
        printf("Failed to init hidapi.");
        return -1;
    }

    if (argc == 3 && strcmp(argv[1], "flash") == 0) {
        if (flash_program(argv[2])) {
            printf("Successfully flashed program: %s\n", argv[2]);
            result = 0;
        } else {
            printf("Failed to flash program: %s\n", argv[2]);
            result = -1;
        }
    } else if (argc >= 2 && strcmp(argv[1], "debug") == 0) {
        hid_device *handle = enter_device_mode(false);
        if (handle == 0) {
            printf("Failed\n");
        } else {
            printf("Success\n");

            uint8_t packet[PACKET_SIZE];
            packet[0] = REQUEST_DEBUG;
            if (send_receive(handle, packet)) {
                for (int i = 0; i < PACKET_SIZE; ++i) {
                    printf("0x%X ", packet[i]);
                }
                printf("\n");
            } else {
                printf("Debug command failed.\n");
            }
        }
    } else {
        printf("Usage: %s flash <path/to/program.bin>\n", argv[0]);
        result = -1;
    }

    /* Free static HIDAPI objects. */
    hid_exit();

#ifdef WIN32
    system("pause");
#endif

    return result;
}
