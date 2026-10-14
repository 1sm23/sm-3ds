#pragma once
// Generated art for the margins of the screens the game draws at 256x224 (docs/wide-scene-extension.md). Files of
// 400x240 bytes, rows from the top: 0 = nothing, else a CGRAM index (the art changes colour with the game's palette,
// as the mode 7 plane does); only the margins left and right of the game's 256 columns are written. Read from the SD
// card's data folder (wide-scenes/NAME.bin: an override) and then from the romfs (wide-scenes/NAME.bin).
#include <stdbool.h>
#include <stdint.h>

enum { kSceneArtPixels = 400 * 240 };

// Reads NAME's file into `out` (kSceneArtPixels bytes). False if there is none or it is the wrong size.
bool SceneArt_Read(const char *name, uint8_t *out);
