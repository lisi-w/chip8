// Include libusb-related headers here.

#include "keyboard.hpp"

Keyboard::Keyboard() {
  // Initialize member variables.
}

Keyboard::~Keyboard() {
  // Clean up, if necessary. For example, you may wanna
  // see if libusb_open has a libusb_close counterpart.
  // Implementing this means that our program can exit gracefully.
}

bool Keyboard::find_keyboard() {
  // Try to initialize a handle and return whether it was successful.

  return true; // Replace me!
}

Keyboard::keys Keyboard::get_keys() {
  keys pressed_keys;
  int poll_status;

  libusb_usb_packet packet;   // Make a packet.
  poll_status poll_keyoard(); //  (fill up packet)

  // Some pseudo code to help place error checking stuff.
  if (poll_status != SUCCESS) {
    pressed_keys.error = poll_status;
    return; // Return early if polling failed.
  }

  // Fill up keys struct. The packet given by libusb is, as you know,
  // complicated and confusing. This class is meant to abstract all that
  // away, so we just set each bool and the bits in keypad
  // to their appropriate values.

  // For instance, your code should have logic like:
  if (keydown_pressed(packet) {
    packet.down = 1;
  } else {
    packet.down = 0;
  }

  // Make sure your code can set MULTIPLE bits if multiple keys are pressed!

  return pressed_keys;
}

