#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

#include "font.hpp"
#include "statemachine.hpp"

const size_t SCALING_FACTOR = 1700 / statemachine::DISPLAY_WIDTH;

std::string mem_of(const statemachine &mach) {
  using namespace std;
  stringstream mem_str;
  mem_str << "mem:";
  unsigned i = 0;
  for (uint8_t val : mach.memory()) {
    if ((i++ % 32) == 0) {
      mem_str << "\n  ";
    }
    mem_str << hex << setw(2) << setfill('0') << (int)val << " ";
  }
  return mem_str.str();
}

/// Reads file contents into CHIP8 memory and places fonts starting at 0x000.
std::optional<std::array<uint8_t, statemachine::MEMORY_SIZE>>
try_load(std::string path) {
  using namespace std;
  array<uint8_t, statemachine::MEMORY_SIZE> ret;
  copy(font.begin(), font.end(), ret.begin());
  ifstream fs(path, std::fstream::in);
  if (!fs.is_open()) {
    return nullopt;
  }

  fs.read(reinterpret_cast<char *>(ret.data() + statemachine::PROG_BEGIN),
          statemachine::MEMORY_SIZE - statemachine::PROG_BEGIN);

  // If the file doesn't fill up memory,
  // fill the remainder with 0's.
  if (fs.eof()) {
    auto mem_prog_begin = ret.begin() + statemachine::PROG_BEGIN + fs.gcount();
    std::fill(mem_prog_begin, ret.end(), 0);
  }

  // Make sure that the file is no larger than the available space.
  if (fs.peek() != ifstream::traits_type::eof()) {
    return nullopt;
  }

  return ret;
}

/// Returns true if it handled a KeyPressed or KeyReleased event.
bool update_keys(uint16_t &keystate, sf::Event &event) {
  if ((event.type != sf::Event::KeyPressed) &&
      (event.type != sf::Event::KeyReleased)) {
    return false;
  }

  uint16_t update;
  switch (event.key.code) {
  case sf::Keyboard::Num1:
    update = 1 << 15;
    break;
  case sf::Keyboard::Num2:
    update = 1 << 14;
    break;
  case sf::Keyboard::Num3:
    update = 1 << 13;
    break;
  case sf::Keyboard::Num4:
    update = 1 << 12;
    break;

  case sf::Keyboard::Q:
    update = 1 << 11;
    break;
  case sf::Keyboard::W:
    update = 1 << 10;
    break;
  case sf::Keyboard::E:
    update = 1 << 9;
    break;
  case sf::Keyboard::R:
    update = 1 << 8;
    break;

  case sf::Keyboard::A:
    update = 1 << 7;
    break;
  case sf::Keyboard::S:
    update = 1 << 6;
    break;
  case sf::Keyboard::D:
    update = 1 << 5;
    break;
  case sf::Keyboard::F:
    update = 1 << 4;
    break;

  case sf::Keyboard::Z:
    update = 1 << 3;
    break;
  case sf::Keyboard::X:
    update = 1 << 2;
    break;
  case sf::Keyboard::C:
    update = 1 << 1;
    break;
  case sf::Keyboard::V:
    update = 1 << 0;
    break;

  default:
    update = 0;
  }

  if (event.type == sf::Event::KeyPressed) {
    keystate |= update;
  } else {
    keystate &= ~update;
  }
  return true;
}

int main(int argc, char **argv) {
  using namespace std;

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <ROM.ch8>\n";
    return 1;
  }

  string path = argv[1];

  if (!path.ends_with(".ch8")) {
    cerr << "path has invalid extension (expected .ch8)\n";
    return 1;
  }

  auto possible_mem = try_load(path);
  if (!possible_mem.has_value()) {
    cerr << "Failed to open " << path << endl;
    return 1;
  }
  statemachine machine(*possible_mem, {.pc = 0x200, .font_begin = 0x000});

  cout << "Successfully loaded " << argv[1] << '\n';

  sf::RectangleShape pixel(sf::Vector2f(SCALING_FACTOR, SCALING_FACTOR));

  sf::RenderWindow window(
      sf::VideoMode(SCALING_FACTOR * statemachine::DISPLAY_WIDTH,
                    SCALING_FACTOR * statemachine::DISPLAY_HEIGHT),
      "CHIP8 Emulator");

  window.setFramerateLimit(60);

  uint16_t keystate = 0;
  int ret = 0;
  while (window.isOpen()) {
    window.clear();

    // Process events
    for (sf::Event event; window.pollEvent(event);) {
      // Close window: exit
      if (event.type == sf::Event::Closed) {
        window.close();
      } else {
        update_keys(keystate, event);
      }
    }

    for (int i = 0; i < (700 / 60); ++i) {
      if (machine.step(keystate, true) != statemachine::NO_ERROR) {
        window.close();
        ret = 1;
        break;
      }
    }
    if (ret != 0) {
      break;
    }

    auto display = machine.display();

    for (size_t x = 0; x < statemachine::DISPLAY_WIDTH; ++x) {
      for (size_t y = 0; y < statemachine::DISPLAY_HEIGHT; ++y) {
        if (display.at(y * (statemachine::DISPLAY_WIDTH / 8) + (x / 8)) &
            (1 << (x & 0x111))) {

          pixel.setPosition(
              sf::Vector2f(x * SCALING_FACTOR, y * SCALING_FACTOR));
          window.draw(pixel);
        }
      }
    }

    window.display();
  }
  return ret;
}

