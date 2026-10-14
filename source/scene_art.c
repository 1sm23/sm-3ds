#include "scene_art.h"

#include <stdio.h>

static bool ReadFile(const char *path, uint16_t *out) {
  FILE *f = fopen(path, "rb");
  if (!f) return false;
  const size_t n = fread(out, sizeof(uint16_t), kSceneArtPixels, f);
  uint8_t extra;
  const bool ok = n == kSceneArtPixels && fread(&extra, 1, 1, f) == 0;
  fclose(f);
  return ok;
}

bool SceneArt_Read(const char *name, uint16_t *out) {
  char path[160];
  snprintf(path, sizeof(path), "sdmc:/3ds/Super Metroid 3DS/wide-scenes/%s.bin", name);
  if (ReadFile(path, out)) return true;
  snprintf(path, sizeof(path), "romfs:/wide-scenes/%s.bin", name);
  return ReadFile(path, out);
}
