#ifndef SWIMP_FONT_H
#define SWIMP_FONT_H

#include <array>
#include <cstdint>

/*
 * Each glyph is represented as an unsigned 64 bit int. Each row of the glyph is
 * left-aligned to the first 5 bits of each byte. The first row is in the lowest
 * byte, with successive rows in higher bytes.
 *
 * Font specified by http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
 */
extern const std::array<uint64_t, 16> font;

#endif // SWIMP_FONT_H
