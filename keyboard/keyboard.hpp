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

	public:
		struct keys {
			int error;
			bool up : 0, down : 0, left : 0, right : 0;
			bool escape : 0, enter : 0;
			bool game1 : 0, game2 : 0, game3 : 0, game4 : 0, game5 : 0, game6 : 0;
			uint16_t keypad;
		};
		Keyboard();
		~Keyboard();
		bool find_keyboard();
		keys get_keys();
};
#endif
