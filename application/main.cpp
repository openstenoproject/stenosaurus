#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <hidapi/hidapi.h>

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

bool send_receive(hid_device *handle, unsigned char * const packet) {
    uint8_t buf[PACKET_SIZE + 1];
    buf[0] = 0;
    memcpy(buf + 1, packet, PACKET_SIZE);

    int res = hid_write(handle, buf, PACKET_SIZE + 1);
    if (res < 0) {
        printf("Failed to send.\n");
        return false;
    }
    //res = hid_read_timeout(handle, packet, PACKET_SIZE,  30 * 1000);
    hid_read(handle, packet, PACKET_SIZE);
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
0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61,0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD };

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
    // Send bootloader request. A one means we are in the bootloader. A zero means we are going to boot into the bootloader so wait and try again.
    // Send erase.
    // send many flash instructions to populate the program
    // call verify on the whole flash
    // reset
    // if there are any errors, report error, try again?

    // Get into bootloader mode.
    int attempts = 5;
    while (true) {
        handle = hid_open(STENOSAURUS_VID, STENOSAURUS_PID, NULL);
        if (handle == 0) {
            printf("Could not find device.\n");
            if (--attempts == 0) break;
            sleep(1000);
        } else {
            memset(packet, 0, PACKET_SIZE);
            packet[0] = REQUEST_BOOTLOADER;
            if (!send_receive(handle, packet)) {
                printf("Failed to communicated with device.\n");
            }
            if (packet[1] != 1) {
                printf("Device is not in bootloader mode.\n");
            }
            if (res >= 0 && packet[1] == 1) {
                break;
            } else {
                hid_close(handle);
                handle = 0;
                if (--attempts == 0) break;
                sleep(1000);
            }
        }
    }

    if (handle == 0) {
        printf("Could not enter bootloader mode.\n");
        return false;
    }

    // Erase current program.
    memset(packet, 0, PACKET_SIZE);
    packet[0] = REQUEST_ERASE_PROGRAM;
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
    make_verify_packet(packet, PROGRAM_MEMORY_SIZE / 4);
    if (!send_receive(handle, packet)) {
        printf("Failed to send verify request.\n");
        return false;
    }
    uint32_t received_crc = packet[1] | (packet[2] << 8) | (packet[3] << 16) | (packet[4] << 24);
    if (received_crc != full_crc) {
        printf("CRC mismatch. Actual: %u, Received: %u\n", full_crc, received_crc);
        return false;
    }

    // reset
    memset(packet, 0, PACKET_SIZE);
    packet[0] = REQUEST_RESET;
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
        hid_device *handle = hid_open(STENOSAURUS_VID, STENOSAURUS_PID, NULL);
        if (handle == 0) {
            printf("Could not find device.\n");
            result = -1;
        } else {
            printf("Found device\n");
            result = 0;
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
