#ifndef SWIMP_FONT_H
#define SWIMP_FONT_H

#include <array>
#include <cstdint>

/*
 * Font specified by http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
 */
const static std::size_t FONT_SPRITE_SIZE = 5;
extern const std::array<uint8_t, FONT_SPRITE_SIZE * 16> font;

#endif // SWIMP_FONT_H
