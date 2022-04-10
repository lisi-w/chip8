#include <iostream>
#include <cstdlib>
#include "keyboard.cpp"

#define ENTER 0x28
#define ESC 0x29

#define LEFT 0x50
#define RIGHT 0x4F

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;


int main()
{

  struct usb_keyboard_packet packet;
  int transferred;
  char ascii_char;

  /* Open the keyboard */
  if (!Keyboard(&endpoint_address)) {
    std::cerr << "Did not find a keyboard\n";
    exit(1);
  }
  keyboard = Keyboard.keyboard;
    
  /* Look for and handle keypresses */
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      uint16_t ctrl = get_control_keys(packet);
      if (ctrl == NULL)  { 
	      char key = get_keys(packet);
	      // send key to game as input
      }
      else if (ctrl == ESC)
	break;
      else if (ctrl == ENTER) {
	      // select game
      }
      else if (ctrl == LEFT) {
	      // move for game selection
      }
      else if (ctrl == RIGHT) {
	      // move for game selection
      }
    }
  }

  return 0;
}


