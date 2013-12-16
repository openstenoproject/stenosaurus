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

int send_receive(hid_device *handle, unsigned char *buf, int buf_size) {
    int res = hid_write(handle, buf, buf_size + 1);
    if (res < 0) {
        printf("Failed to send.\n");
        return -1;
    }
    res = hid_read_timeout(handle, buf, buf_size,  10 * 1000);
    if (res <= 0) {
        printf("failed to receive.\n");
        return -1;
    }
    return res;
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

uint32_t compute_crc(uint8_t *buf, uint32_t size) {
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

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    int res;
    unsigned char buf[65];
    #define MAX_STR 255
    wchar_t wstr[MAX_STR];
    hid_device *handle;

    if (hid_init()) {
        printf("Failed to init hidapi.");
        return -1;
    }

    // Open the device using the VID, PID,
    // and optionally the Serial number.
    ////handle = hid_open(0x4d8, 0x3f, L"12345");
    handle = hid_open(0x6666, 1, NULL);
    if (!handle) {
        printf("unable to open device\n");
        return -2;
    }

    buf[0] = 0; // HID report id.
    buf[1] = 4; // action verify
    buf[2] = 1; // num words
    buf[3] = 0; // address 0
    buf[4] = 0; // 1
    buf[5] = 0; // 2
    buf[6] = 0; // 3
    buf[7] = 0; // word 0
    buf[8] = 0; // 1
    buf[9] = 0; // 2
    buf[10] = 0; // 3
    res = send_receive(handle, buf, 64);
    if (res < 0) {
        printf("Failed to send_receive.\n");
    }

    if (buf[0] == 1) {
        printf("Success\n");
    } else if (buf[0] == 2) {
        printf("Error\n");
    }

    printf("Data: ");
    for (int i = 0; i < 64; ++i) {
        printf("0x%02X ", buf[i]);
    }
    printf("\n");

    uint32_t crc = buf[1] | (buf[2] << 8) | (buf[3] << 16) | (buf[4] << 24);
    printf("board crc: 0x%08x\n", crc);

    uint8_t data[] = {0, 0, 0, 0};
    uint32_t my_crc = compute_crc(data, 4);
    printf("my crc: 0x%08x\n", my_crc);


    hid_close(handle);

    /* Free static HIDAPI objects. */
    hid_exit();

#ifdef WIN32
    system("pause");
#endif

    return 0;
}
