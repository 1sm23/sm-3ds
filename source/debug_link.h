#pragma once
// Remote control of the running game for DEBUG_TOOLS builds (docs/debug-tools.md, "Remote
// control"), carried as SITE commands over the in-game FTP server so there is one port and one
// switch:
//
//   SITE KEYS <A+B+LEFT|NONE> [frames]   hold 3DS buttons for N logic frames (default 1)
//   SITE TOUCH <x> <y>                    tap the bottom screen (320x240)
//   SITE SHOT                             write debug/shot-top.bmp and debug/shot-bottom.bmp
//   SITE SAVESTATE [n]                    a new save state, or over number n (1 = newest)
//   SITE LOADSTATE [n]                    load save state n (1 = newest, the default)
//   SITE DELSTATE <n>                     delete save state n
//   SITE STATES                           the list: n=area/room ...
//   SITE DUMP                             take a screen dump set (the DEBUG TOOLS SCREEN DUMP)
//   SITE SWITCH <name>                    cycle a DEBUG TOOLS / OPTIONS value (RENDERER, WIDE...)
//   SITE STATE                            one line: frame, in game, area, room, Samus, energy
//   SITE PEEK <address> <length>          hex of up to 256 bytes of this process's memory
//   SITE POKE <address> <hex bytes>       write them (main thread, between frames)
//
// tools/sm-link/ is the PC client. Anything that touches the game runs on the main thread, in
// DebugLink_Poll at the top of the frame; the FTP thread waits for it.
#include <stdbool.h>
#include <stdint.h>

// FTP thread. Handles `verb` (upper case) with `args`; writes the whole reply line, code
// included, into `reply`. False when the verb is not ours.
bool DebugLink_Command(const char *verb, const char *args, char *reply, unsigned cap);

// Main thread, once per frame right after the input scan: adds the injected buttons to the
// HID masks `down` / `up` (KEY_* bits) and runs the queued request, if any. `frame` counts
// the game's logic frames.
void DebugLink_Poll(uint32_t *down, uint32_t *up, uint64_t frame);

// Main loop: a save state job (req_save_state / req_load_state) just finished.
void DebugLink_StateDone(bool ok);
