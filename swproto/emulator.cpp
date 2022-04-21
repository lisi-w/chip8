#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <bitset>
#include <iostream>

#include "statemachine.hpp"

const size_t SCALING_FACTOR = 1200 / statemachine::DISPLAY_WIDTH;

/* Returne true if it handled a KeyPressed or KeyReleased event. */
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

  cout << "Got " << argv[1];

  sf::RectangleShape pixel(sf::Vector2f(SCALING_FACTOR, SCALING_FACTOR));

  // statemachine machine({});
  // Create the main window
  sf::RenderWindow window(
      sf::VideoMode(SCALING_FACTOR * statemachine::DISPLAY_WIDTH,
                    SCALING_FACTOR * statemachine::DISPLAY_HEIGHT),
      "CHIP8 Emulator");
  // Start the game loop
  uint16_t keystate = 0;
  while (window.isOpen()) {
    // Clear screen
    window.clear();

    // Process events
    sf::Event event;
    while (window.pollEvent(event)) {
      // Close window: exit
      if (event.type == sf::Event::Closed) {
        window.close();
      } else if (update_keys(keystate, event)) {
        cerr << std::bitset<16>(keystate) << ' ';
      }
    }

    // auto display = ma

    for (size_t x = 0; x < statemachine::DISPLAY_WIDTH; ++x) {
      for (size_t y = 0; y < statemachine::DISPLAY_HEIGHT; ++y) {
        if (x & y & 1) {
          pixel.setPosition(
              sf::Vector2f(x * SCALING_FACTOR, y * SCALING_FACTOR));
          window.draw(pixel);
        }
      }
    }

    window.display();
  }
  return EXIT_SUCCESS;
}

