#include <iostream>
#include <cstdlib>
#include "keyboard.hpp"

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

	uint16_t get_keys() {
		uint16_t key = 0x29;
		return key;
	}

	uint16_t get_control_keys() {
		uint16_t key = 0x29;
		return key;
	}
}
