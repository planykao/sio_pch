#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#include <pin_list.h>

void Pin_list(char *chip_model, unsigned int *index, unsigned int *type, unsigned int *bank)
{
	int pin = *index;

	if (strcmp("NCT6776D", chip_model) == 0 || strcmp("NCT6776F", chip_model) == 0) {
		switch(pin) {
			case 1:
				*index = 0x23;
				*bank = 0;
				break;
			case 46:
				*index = 0x50;
				*bank = 5;
				break;
			case 99:
				*index = 0x51;
				*bank = 5;
				break;
			case 103:
				*index = 0x25;
				*bank = 0;
				break;
			case 104:
				*index = 0x24;
				*bank = 0;
				break;
			case 105:
				*index = 0x21;
				*bank = 0;
				break;
			case 106:
				*index = 0x22;
				*bank = 0;
				break;
			case 107:
				*index = 0x20;
				*bank = 0;
				break;
			case 109:
				*index = 0x26;
				*bank = 0;
				break;
			case 124:
				*index = 0x58;
				*bank = 6;
				break;
			case 126:
				*index = 0x56;
				*bank = 6;
				break;
			case 1281:
				*index = 0x50;
				*bank = 1;
				break;
			case 1282:
				*index = 0x2B;
				*bank = 6;
				break;
		}
	} else if (strcmp("NCT6779D", chip_model) == 0) {
		switch(pin) {
			case 3:
				*index = 0xC4;
				*bank = 4;
				break;
			case 4:
				*index = 0xC6;
				*bank = 4;
				break;
			case 99:
				*index = 0x88;
				*bank = 4;
				break;
			case 104:
				*index = 0x84;
				*bank = 4;
				break;
			case 105:
				*index = 0x81;
				*bank = 4;
				break;
			case 106:
				*index = 0x8C;
				*bank = 4;
				break;
			case 107:
				*index = 0x8D;
				*bank = 4;
				break;
			case 109:
				*index = 0x80;
				*bank = 4;
				break;
			case 111:
				*index = 0x86;
				*bank = 4;
				break;
			case 112:
				*index = 0x20;
				*bank = 7;
				break;
			case 113:
				*index = 0x90;
				*bank = 4;
				break;
			case 114:
				*index = 0x8A;
				*bank = 4;
				break;
			case 115:
				*index = 0x8B;
				*bank = 4;
				break;
			case 116:
				*index = 0x8E;
				*bank = 4;
				break;
			case 124:
				*index = 0xC2;
				*bank = 4;
				break;
			case 126:
				*index = 0xC0;
				*bank = 4;
				break;
		}
	} else if (strncmp("F718", chip_model, 4) == 0) {
		*bank = 0;
		switch(pin) {
			case 21:
				*index = 0xA0;
				break;
			case 23:
				*index = 0xB0;
				break;
			case 25:
				*index = 0xC0;
				break;
			case 89:
				*index = 0x76;
				break;
			case 90:
				*index = 0x74;
				break;
			case 91:
				*index = 0x72;
				break;
			case 93:
				*index = 0x26;
				break;
			case 94:
				*index = 0x25;
				break;
			case 95:
				*index = 0x24;
				break;
			case 96:
				*index = 0x23;
				break;
			case 97:
				*index = 0x22;
				break;
			case 98:
				*index = 0x21;
				break;
		}

		if (strncmp("F7186", chip_model, 5)  ==  0) {
			switch(pin) {
				case 4:
				case 37:
					*index = 0x20;
					break;
				case 58:
					*index = 0x7E;
					break;
				case 68:
					*index = 0x27;
					break;
				case 86:
					*index = 0x28;
					break;
			}
		}

		if (strcmp("F71889AD", chip_model)  ==  0) {
			switch(pin) {
				case 1:
				case 35:
					*index = 0x20;
					break;
				case 44:
					*index = 0x78;
					break;
				case 65:
					*index = 0x27;
					break;
				case 82:
					*index = 0x28;
					break;
			}
		}
	} else if (strcmp("AST1300", chip_model)  ==  0) {
		switch(pin) {
			case 1: break;
		}
	}
}
