#include <iostream>
#include <cstdlib>
#include "keyboard.hpp"

#define ENTER 0x28
#define ESC 0x29
#define BACKSPACE 0x2A

#define LEFT 0x50
#define RIGHT 0x4F

class Keyboard {
	// define constructor
	Keyboard(uint8_t *endpoint_addr) {
		struct libusb_device_handle *keyboard = NULL;
		uint8_t *endpoint_address = endpoint_addr;
	}

	bool find_keyboard() {
		libusb_device **devs;
   		struct libusb_device_descriptor desc;
		ssize_t num_devs, d;
		uint8_t i, k;

		/* Start the library */
		if ( libusb_init(NULL) < 0 ) {
			std::cerr << "Error: libusb_init failed\n";
			return false;
		}

		/* Enumerate all the attached USB devices */
		if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
			std::cerr << "Error: libusb_get_device_list failed\n";
			return false;
		}

		/* Look at each device, remembering the first HID device that speaks
		*      the keyboard protocol */

		for (d = 0 ; d < num_devs ; d++) {
			libusb_device *dev = devs[d];
			if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
				std::cerr << "Error: libusb_get_device_descriptor failed\n";
				return false;
			}
			if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
				struct libusb_config_descriptor *config;
			        libusb_get_config_descriptor(dev, 0, &config);
				for (i = 0 ; i < config->bNumInterfaces ; i++)
					for ( k = 0 ; k < config->interface[i].num_altsetting ; k++ ) {
						const struct libusb_interface_descriptor *inter =
						config->interface[i].altsetting + k ;
						if ( inter->bInterfaceClass == LIBUSB_CLASS_HID &&
							inter->bInterfaceProtocol == USB_HID_KEYBOARD_PROTOCOL) {
							int r;
							if ((r = libusb_open(dev, &keyboard)) != 0) {
								std::cerr << "Error: libusb_open failed: %d\n", r;
								return false;
							}
							if (libusb_kernel_driver_active(keyboard,i))
								libusb_detach_kernel_driver(keyboard, i);
							libusb_set_auto_detach_kernel_driver(keyboard, i);
							if ((r = libusb_claim_interface(keyboard, i)) != 0) {
								std::cerr << "Error: libusb_claim_interface failed: %d\n", r;
								return false;
							}
							*endpoint_address = inter->endpoint[0].bEndpointAddress;
							goto found;
						}
					}
			}
		}

		found:
			libusb_free_device_list(devs, 1);
			return true;
	}

	char get_keys(struct usb_keyboard_packet packet) {
		/* maps keyboard to chip8 keys as follows:
		 * 1 2 3 4        1 2 3 C
		 * q w e r        4 5 6 D
		 * a s d f  --->  7 8 9 E
		 * z x c v        A 0 B F
		*/
		switch (packet.keycode[0]) {
			case 0x1E :
				return '1';
			case 0x1F :
				return '2';
			case 0x20 :
				return '3';
			case 0x21 :
				return 'C';
			case 0x14 :
				return '4';
			case 0x1A :
				return '5';
			case 0x08 :
				return '6';
			case 0x15 :
				return 'D';
			case 0x04 :
				return '7';
			case 0x16 :
				return '8';
			case 0x07 :
				return '9';
			case 0x09 :
				return 'E';
			case 0x1D :
				return 'A';
			case 0x1B :
				return '0';
			case 0x06 :
				return 'B';
			case 0x19 :
				return 'F';
			default :
				return NULL;
		}
	}

	uint16_t get_control_keys(struct usb_keyboard_packet packet) {
		uint16_t key = NULL;
		if (packet.keycode[0] == ESC) { /* ESC pressed? */
			key = ESC;
		}
		else if (packet.keycode[0] == 0) key = NULL;
		else if (packet.keycode[0] == ENTER) {
			key = ENTER;
		}
		else if (packet.keycode[0] == LEFT) {
			key = LEFT;
		}
		else if (packet.keycode[0] == RIGHT) {
			key = RIGHT;
		}
		return key;
	}
}
