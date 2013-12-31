
#include "usb.h"

void print(char* str) {
	int len = 0;
	char* b = str;
	while (*b) { b++; len++; }
	while (serial_usb_send_data(str, len) == 0);
}

static char nibble_to_hex_char(uint8_t b) {
	// TODO: The number and letter parts could be collapesed with a base plus offset.
	switch (b & 0xF) {
		case 0: return '0';
		case 1: return '1';
		case 2: return '2';
		case 3: return '3';
		case 4: return '4';
		case 5: return '5';
		case 6: return '6';
		case 7: return '7';
		case 8: return '8';
		case 9: return '9';
		case 10: return 'A';
		case 11: return 'B';
		case 12: return 'C';
		case 13: return 'D';
		case 14: return 'E';
		default: return 'F';
	}
}

void print_word(uint32_t word) {
	char buf[11];
	char *b = buf;
	*b++ = '0';
	*b++ = 'x';
	*b++ = nibble_to_hex_char(word >> (3 * 8 + 4));
	*b++ = nibble_to_hex_char(word >> (3 * 8));
	*b++ = nibble_to_hex_char(word >> (2 * 8 + 4));
	*b++ = nibble_to_hex_char(word >> (2 * 8));
	*b++ = nibble_to_hex_char(word >> (8 + 4));
	*b++ = nibble_to_hex_char(word >> 8);
	*b++ = nibble_to_hex_char(word >> 4);
	*b++ = nibble_to_hex_char(word);
	*b++ = 0;
	print(buf);
}

void print_arg1(char *str, uint32_t word) {
	print(str);
	print_word(word);
	print("\r\n");
}