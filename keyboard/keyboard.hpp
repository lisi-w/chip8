#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <cstdint>

class Keyboard {
private:
  // PUT STATE VARIABLES HERE.
  // For example, put the device handle here.
  struct libusb_device_handle *handle;

public:
  struct keys {
    int error;
    bool up : 1, down : 1, left : 1, right : 1;
    bool escape : 1, enter : 1;
    // For 16 normal CHIP8 keys.
    uint16_t keypad;
  };

  /** You may not need either or both of the constructor or destructor.
   *  It depends entirely on the kinds of state variables libusb
   *  requires.
   */
  Keyboard();
  ~Keyboard();

  /** Should try to do a libopen(handle) or whatever and return
   *  true if a keyboard was found.
   */
  bool find_keyboard();

  /** get_keys should poll the keyboard with libusb_device_handle
   *  and then use the resulting usb_keyboard_packet to pack a new
   *  "keys" struct and return that.
   *
   *  If polling returns an error, it should set "error" in the return struct
   *  to a non-zero value (the same one the polling function returns!)
   */
  keys get_keys();
};

#endif // KEYBOARD_HPP
