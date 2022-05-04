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
  keyboard = Keyboard.keyboard;
  if (!keyboard.find_keyboard()) {
    std::cerr << "Did not find a keyboard\n";
    exit(1);
  }
  std::cout << "Keyboard initialized\n";
    
  /* Look for and handle keypresses */
  for (;;) {
	keyboard.keys = keyboard.get_keys()
	std::cout << "%d" << keyboard.keys.keypad;
  }

  return 0;
}


