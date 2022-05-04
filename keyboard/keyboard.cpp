#include <iostream>
#include <cstdlib>
#include "keyboard.hpp"

#define ENTER 0x28
#define ESC 0x29
#define BACKSPACE 0x2A

#define LEFT 0x50
#define RIGHT 0x4F
#define UP 0x52
#define DOWN 0x51

Keyboard::Keyboard(uint8_t *endpoint_addr) {
	struct libusb_device_handle *keyboard = NULL;
	uint8_t *endpoint_address = endpoint_addr;
}
Keyboard::~Keyboard();

bool Keyboard::find_keyboard() {
	libusb_device **devs;
	struct libusb_device_descriptor desc;
	ssize_t num_devs, d;
	uint8_t i, k;

	/* Start the library */
	if ( libusb_init(NULL) < 0 ) {
		// std::cerr << "Error: libusb_init failed\n";
		return false;
	}
	/* Enumerate all the attached USB devices */
	if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
		// std::cerr << "Error: libusb_get_device_list failed\n";
		return false;
	}
	/* Look at each device, remembering the first HID device that speaks
	*      the keyboard protocol */
	for (d = 0 ; d < num_devs ; d++) {
		libusb_device *dev = devs[d];
		if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
			// std::cerr << "Error: libusb_get_device_descriptor failed\n";
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
							// std::cerr << "Error: libusb_open failed: %d\n", r;
							return false;
						}
						if (libusb_kernel_driver_active(keyboard,i))
							libusb_detach_kernel_driver(keyboard, i);
						libusb_set_auto_detach_kernel_driver(keyboard, i);
						if ((r = libusb_claim_interface(keyboard, i)) != 0) {
							// std::cerr << "Error: libusb_claim_interface failed: %d\n", r;
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

Keyboard::keys Keyboard::get_keys() {
	keys pressed_keys;
	libusb_interrupt_transfer(keyboard, endpoint_address,
                              (unsigned char *) &packet, sizeof(packet),
                              &transferred, 0);
	uint16_t keypad;
    	if (transferred == sizeof(packet)) {
		for (int i = 0; i < 6; i++) {

			/* maps keyboard to chip8 keys as follows:
			 * 1 2 3 4        1 2 3 C
			 * q w e r        4 5 6 D
			 * a s d f  --->  7 8 9 E
			 * z x c v        A 0 B F
			*/
			switch (packet.keycode[i]) {
				case 0x1E :
					keypad = 1 << 0x1;
					break;
				case 0x1F :
					keypad = 1 << 0x2;
					break;
				case 0x20 :
					keypad = 1 << 0x3;
					break;
				case 0x21 :
					keypad =  1 << 0xC;
					break;
				case 0x14 :
					keypad = 1 << 0x4;
					break;
				case 0x1A :
					keypad = 1 << 0x5;
					break;
				case 0x08 :
					keypad = 1 << 0x6;
					break;
				case 0x15 :
					keypad = 1 << 0xD;
					break;
				case 0x04 :
					keypad = 1 << 0x7;
					break;
				case 0x16 :
					keypad = 1 << 0x8;
					break;
				case 0x07 :
					keypad = 1 << 0x9;
					break;
				case 0x09 :
					keypad = 1 << 0xE;
					break;
				case 0x1D :
					keypad = 1 << 0xA;
					break;
				case 0x1B :
					keypad = 1 << 0x0;
					break;
				case 0x06 :
					keypad = 1 << 0xB;
					break;
				case 0x19 :
					keypad = 1 << 0xF;
					break;
				case ESC : 
					pressed_keys.escape = 1;
					break;
				case ENTER :
					pressed_keys.enter = 1;
					break;
				case LEFT :
					pressed_keys.left = 1;
					break;
				case RIGHT :
					pressed_keys.right = 1;
					break;
				case UP :
					pressed_keys.up = 1;
					break;
				case DOWN :
					pressed_keys.down = 1;
					break;
				case FIVE :
					pressed_keys.game1 = 1;
					break;
				case SIX :
					pressed_keys.game2 = 1;
					break;
				case SEVEN :
					pressed_keys.game3 = 1;
					break;
				case EIGHT :
					pressed_keys.game4 = 1;
					break;
				case NINE :
					pressed_keys.game5 = 1;
					break;
				case ZERO :
					pressed_keys.game6 = 1;
					break;
				default :
					if !(keypad) keypad = 0;
			}
		}
	}
	pressed_keys.keypad = keypad;
	return pressed_keys;
}
