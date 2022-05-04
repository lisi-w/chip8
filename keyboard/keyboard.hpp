#ifndef _USBKEYBOARD_H
#define _USBKEYBOARD_H

#include <libusb-1.0/libusb.h>

#define USB_HID_KEYBOARD_PROTOCOL 1

/* Modifier bits */
#define USB_LCTRL  (1 << 0)
#define USB_LSHIFT (1 << 1)
#define USB_LALT   (1 << 2)
#define USB_LGUI   (1 << 3)
#define USB_RCTRL  (1 << 4)
#define USB_RSHIFT (1 << 5)
#define USB_RALT   (1 << 6)
#define USB_RGUI   (1 << 7)

struct usb_keyboard_packet {
	  uint8_t modifiers;
	    uint8_t reserved;
	      uint8_t keycode[6];
};


class Keyboard {

	private:
		struct libusb_device_handle *keyboard;
		uint8_t endpoint_addr;
		uint8_t endpoint_address = &endpoint_addr;

	public:
		struct keys {
			int error;
			bool up : 1, down : 1, left : 1, right : 1;
			bool escape : 1, enter : 1;
			bool game1 : 1, game2 : 1, game3 : 1, game4 : 1, game5 : 1, game6 : 1;
			uint16_t keypad;
		};
		Keyboard();
		~Keyboard();
		bool find_keyboard();
		keys get_keys();
};
#endif
