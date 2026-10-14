#pragma once
// Generated art for the margins of the screens the game draws at 256x224 (docs/wide-scene-extension.md). Files of
// 400x240 RGBA5551 (little endian, rows from the top; the game's own 256x224 at (72, 8) left empty), read from the
// SD card's data folder (wide-scenes/NAME.bin: an override) and then from the romfs (wide-scenes/NAME.bin).
#include <stdbool.h>
#include <stdint.h>

enum { kSceneArtPixels = 400 * 240 };

// Reads NAME's file into `out` (kSceneArtPixels texels). False if there is none or it is the wrong size.
bool SceneArt_Read(const char *name, uint16_t *out);
