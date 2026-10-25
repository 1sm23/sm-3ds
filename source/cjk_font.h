#pragma once

#include <stdint.h>

typedef struct {
  uint32_t code;
  uint8_t ui[7];
  uint8_t small[8];
  uint8_t large[16];
} CjkGlyph;

const CjkGlyph *CjkFont_Find(unsigned code);

