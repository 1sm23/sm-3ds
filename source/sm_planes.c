#include "sm_planes.h"

#include <stdint.h>
#include <string.h>

#include "src/types.h"
#include "src/variables.h"
#include "src/ida_types.h"
#include "sm_wide.h"
#include "stereo_depth.h"

typedef struct {
  uint16_t room;
  uint8_t layer, prio, plane;
} LayerRule;

typedef struct {
  uint16_t room, bx, by, block;
  uint8_t layer, corners, plane;   // for SM_TILE_PRIO `plane` is the priority
} TileFix;

// source/sm_plane_fixes.inc: SM_LAYER_PLANE is a rule for a whole layer, SM_PLANE_FIX one block's tiles (corners).
#define SM_PLANE_FIX(room, layer, bx, by, block, corners, plane)
#define SM_TILE_PRIO(room, layer, bx, by, block, corners, prio)
#define SM_LAYER_PLANE(room, layer, prio, plane) { room, layer, prio, plane },
static const LayerRule kRules[] = {
#include "sm_plane_fixes.inc"
  { 0, 0, 0, 0 }   // never matches (room 0 is not a room), and keeps the array non-empty
};
#undef SM_PLANE_FIX
#undef SM_TILE_PRIO
#undef SM_LAYER_PLANE

#define SM_PLANE_FIX(room, layer, bx, by, block, corners, plane) { room, bx, by, block, layer, corners, plane },
#define SM_TILE_PRIO(room, layer, bx, by, block, corners, prio)
#define SM_LAYER_PLANE(room, layer, prio, plane)
static const TileFix kFixes[] = {
#include "sm_plane_fixes.inc"
  { 0, 0, 0, 0, 0, 0, 0 }
};
#undef SM_PLANE_FIX
#undef SM_TILE_PRIO
#undef SM_LAYER_PLANE
enum { kFixCount = (int)(sizeof(kFixes) / sizeof(kFixes[0])) - 1 };

#define SM_PLANE_FIX(room, layer, bx, by, block, corners, plane)
#define SM_TILE_PRIO(room, layer, bx, by, block, corners, prio) { room, bx, by, block, layer, corners, prio },
#define SM_LAYER_PLANE(room, layer, prio, plane)
static const TileFix kPrios[] = {
#include "sm_plane_fixes.inc"
  { 0, 0, 0, 0, 0, 0, 0 }
};
#undef SM_PLANE_FIX
#undef SM_TILE_PRIO
#undef SM_LAYER_PLANE
enum { kPrioCount = (int)(sizeof(kPrios) / sizeof(kPrios[0])) - 1 };

typedef struct { uint16_t room; uint8_t cap; } SpriteCap;
#define SM_SPRITE_CAP(room, cap) { room, cap },
static const SpriteCap kSpriteCaps[] = {
#include "sm_sprite_prio.inc"
  { 0, 3 }
};
#undef SM_SPRITE_CAP

typedef struct { uint16_t room; uint8_t first, last, plane, kind, y0, y1; } M7Rule;   // kind: 0 tiles, 1 solid, 2 fill, 3 across, 4 cells, 5 no fill
#define SM_M7_TILES(room, first, last, plane) { room, first, last, plane, 0, 0, 0 },
#define SM_M7_SOLID(room, plane) { room, 0, 0, plane, 1, 0, 0 },
#define SM_M7_FILL(room, reach) { room, 0, 0, reach, 2, 0, 0 },
#define SM_M7_FILL_ACROSS(room, x0, y0, x1, y1) { room, x0, x1, 0, 3, y0, y1 },
#define SM_M7_CELLS(room, x0, y0, x1, y1, plane) { room, x0, x1, plane, 4, y0, y1 },
#define SM_M7_NO_FILL(room, x0, y0, x1, y1) { room, x0, x1, 0, 5, y0, y1 },
static const M7Rule kM7Rules[] = {
#include "sm_m7_planes.inc"
  { 0, 0, 0, 0, 0, 0, 0 }
};
#undef SM_M7_TILES
#undef SM_M7_SOLID
#undef SM_M7_FILL
#undef SM_M7_FILL_ACROSS
#undef SM_M7_CELLS
#undef SM_M7_NO_FILL

int SmPlanes_Mode7Key(void) {
  for (int i = 0; kM7Rules[i].room; i++)
    if (kM7Rules[i].room == room_ptr) return room_ptr;
  return 0;
}

int SmPlanes_Mode7BasePlane(void) {
  const int p = SmPlanes_LayerRule(5, 0);
  return p >= 0 ? p : kStereoPlay;   // StereoDepth_Plane's for Mode 7
}

int SmPlanes_Mode7FillReach(void) {
  for (int i = 0; kM7Rules[i].room; i++)
    if (kM7Rules[i].room == room_ptr && kM7Rules[i].kind == 2) return kM7Rules[i].plane;
  return 0;
}

int SmPlanes_Mode7FillMode(int cell) {
  const int cx = cell & 127, cy = cell >> 7;
  for (int i = 0; kM7Rules[i].room; i++) {   // the first area that holds the cell, in file order
    const M7Rule *r = &kM7Rules[i];
    if (r->room == room_ptr && (r->kind == 3 || r->kind == 5) && cx >= r->first && cx <= r->last && cy >= r->y0 &&
        cy <= r->y1)
      return r->kind == 3 ? 1 : -1;
  }
  return 0;
}

int SmPlanes_Mode7CellPlane(int cell, int tile) {
  const uint16_t room = room_ptr;
  const int cx = cell & 127, cy = cell >> 7;
  for (int i = 0; kM7Rules[i].room; i++) {   // cell rules first, then tile rules, in file order
    const M7Rule *r = &kM7Rules[i];
    if (r->room == room && r->kind == 4 && cx >= r->first && cx <= r->last && cy >= r->y0 && cy <= r->y1) return r->plane;
  }
  for (int i = 0; kM7Rules[i].room; i++) {
    const M7Rule *r = &kM7Rules[i];
    if (r->room == room && r->kind == 0 && tile >= r->first && tile <= r->last) return r->plane;
  }
  for (int i = 0; kM7Rules[i].room; i++) {
    const M7Rule *r = &kM7Rules[i];
    if (r->room != room || r->kind != 1) continue;
    // The plane drawn 1:1 with the room (the Ceres shaft): cell (cx, cy) lies on block (cx / 2, cy / 2).
    const int bx = (cell & 127) >> 1, by = cell >> 8, w = room_width_in_blocks, h = room_height_in_blocks;
    if (bx < w && by < h && (level_data[by * w + bx] >> 12) == 8) return r->plane;
  }
  return -1;
}

int SmPlanes_SpriteCap(void) {
  for (int i = 0; kSpriteCaps[i].room; i++)
    if (kSpriteCaps[i].room == room_ptr) return kSpriteCaps[i].cap;
  return 3;
}

int SmPlanes_RuleCount(void) { return (int)(sizeof(kRules) / sizeof(kRules[0])) - 1; }

int SmPlanes_LayerRule(int layer, int prio) {
  const uint16_t room = room_ptr;
  for (int i = 0; i < SmPlanes_RuleCount(); i++)
    if (kRules[i].room == room && kRules[i].layer == layer && kRules[i].prio == prio) return kRules[i].plane;
  return -1;
}

// The fixes of the room the game is in, found again whenever the room changes.
static uint16_t g_fix_room;
static int g_fix_n;
static const TileFix *g_fix[kFixCount + 1];
static int g_prio_n;
static const TileFix *g_prio[kPrioCount + 1];

static void FindRoomFixes(void) {
  g_fix_room = room_ptr;
  g_fix_n = g_prio_n = 0;
  for (int i = 0; i < kFixCount; i++)
    if (kFixes[i].room == g_fix_room) g_fix[g_fix_n++] = &kFixes[i];
  for (int i = 0; i < kPrioCount; i++)
    if (kPrios[i].room == g_fix_room) g_prio[g_prio_n++] = &kPrios[i];
}

int SmPlanes_RoomHasRules(void) {
  const uint16_t room = room_ptr;
  for (int i = 0; i < SmPlanes_RuleCount(); i++)
    if (kRules[i].room == room) return 1;
  for (int i = 0; i < kFixCount; i++)
    if (kFixes[i].room == room) return 1;
  for (int i = 0; i < kPrioCount; i++)
    if (kPrios[i].room == room) return 1;
  return 0;
}

int SmPlanes_SlotPlanes(int layer, int tw, int th, uint8_t *grid) {
  if (g_fix_room != room_ptr) FindRoomFixes();
  if ((!g_fix_n && !g_prio_n) || (layer != 1 && layer != 2)) return 0;
  const int w = room_width_in_blocks, h = room_height_in_blocks;
  // Level data: BG1's blocks from $7F:0002; BG2's from $7F:9602 only when it scrolls with the level.
  // The game (and SmWide's FillLayer) puts the block at level column bx in tilemap block (vx0 + bx - lx0), where lx0 is the
  // block of the layer's position and vx0 that of the position plus the scroll offset bg1_x_offset (bg2_x_scroll): after a
  // door the offset is the screen the room was entered at, not 0.
  const uint16_t *level;
  int cam_x, cam_y, vx0, vy0;
  if (layer == 1) {
    level = (const uint16_t *)(g_ram + 0x10002), cam_x = (int16_t)layer1_x_pos, cam_y = (int16_t)layer1_y_pos;
    vx0 = (int16_t)(bg1_x_offset + layer1_x_pos) >> 4, vy0 = (int16_t)(bg1_y_offset + layer1_y_pos) >> 4;
  } else {
    if (layer2_scroll_x & 1) return 0;
    level = (const uint16_t *)(g_ram + 0x19602), cam_x = (int16_t)layer2_x_pos, cam_y = (int16_t)layer2_y_pos;
    vx0 = (int16_t)(bg2_x_scroll + layer2_x_pos) >> 4, vy0 = (int16_t)(bg2_y_scroll + layer2_y_pos) >> 4;
  }
  const int lx0 = cam_x >> 4, ly0 = cam_y >> 4;
  const int wb = tw >> 1, hb = th >> 1;   // blocks the tilemap holds
    // With WIDE the view is wider (and taller) than 256x224 and leans off a room's edge: centre on what is shown.
  int ml, mr, hud_x, bg2_dx, et, eb, hud_y;
  SmWide_Margins(&ml, &mr, &hud_x, &bg2_dx);
  SmWide_Rows(&et, &eb, &hud_y);
  const int x0 = ((cam_x + 128 + (mr - ml) / 2) >> 4) - (wb >> 1), y0 = ((cam_y + 112 + (eb - et) / 2) >> 4) - (hb >> 1);
  memset(grid, 0, (size_t)th * 64);
  int n = 0;
  // Two kinds of entry: planes (low nibble of the slot: StereoPlane + 1) and render priorities (high nibble: priority + 1).
  for (int kind = 0; kind < 2; kind++) {
    const int count = kind ? g_prio_n : g_fix_n;
    for (int i = 0; i < count; i++) {
      const TileFix *f = kind ? g_prio[i] : g_fix[i];
      if (f->layer != layer || f->bx < x0 || f->bx >= x0 + wb || f->by < y0 || f->by >= y0 + hb) continue;
      if (f->bx >= w || f->by >= h || level[f->by * w + f->bx] != f->block) continue;
      for (int c = 0; c < 4; c++) {
        if (!(f->corners >> c & 1)) continue;
        uint8_t *slot = &grid[(((vy0 + f->by - ly0) * 2 + (c >> 1)) & (th - 1)) * 64 + (((vx0 + f->bx - lx0) * 2 + (c & 1)) & (tw - 1))];
        if (!*slot) n++;
        *slot = kind ? (uint8_t)((*slot & 15) | (f->plane + 1) << 4) : (uint8_t)((*slot & 0xf0) | (f->plane + 1));
      }
    }
  }
  return n;
}

// States not listed are flat.
StereoScreen SmPlanes_Screen(void) {
  switch (game_state) {
  case kGameState_0_Reset: case kGameState_1_OpeningCinematic: return kStereoScreenTitle;
  case kGameState_4_FileSelectMenus: return kStereoScreenMenu;
  case kGameState_13_Pausing: case kGameState_14_Paused: case kGameState_15_Paused:
  case kGameState_16_Unpausing: case kGameState_17_Unpausing: return kStereoScreenPause;
  case kGameState_26_GameOverMenu: return kStereoScreenGameOver;
  case kGameState_30_IntroCinematic: return kStereoScreenIntro;
  default: return kStereoScreenFlat;
  }
}
