// Automatic bomb jump (OPTIONS -> CONTROLS -> "?", config.ini `auto_bomb_jump`): while the player
// holds the bomb button in the morph ball, bombs are laid at the moments that keep Samus climbing
// (the infinite bomb jump), worked out from the game's own bomb and fall rules (auto_bomb.c). It
// only ever releases and presses the button, as a player could.
#pragma once

#include <stdbool.h>

bool AutoBomb_On(void);
void AutoBomb_Set(bool on);

// `inputs` (the frontend's pad bits, after ModernControls_Translate) as the game must see them
// this frame. Called once a game frame, before it runs.
int AutoBomb_Translate(int inputs);
