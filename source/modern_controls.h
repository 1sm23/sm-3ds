// The modern control scheme (OPTIONS -> CONTROLS, issue #32, docs/modern-controls.md): Samus
// runs with B, L aims, R + a button uses a weapon, SELECT only switches missiles / super missiles.
// It lives here, in front of the game: the game only ever sees SNES pad bits (through its own
// button configuration) and item selections asked through g_rtl_hud_select, so its RAM holds what
// a SNES pad could produce and RetroAchievements, saves and states are not affected.
#pragma once

#include <stdbool.h>

// The scheme in use (config.ini `controls`); switching asks the game for no item.
bool ModernControls_On(void);
void ModernControls_Set(bool on);

// The missile kind R + X fires: kSmHudMissiles or kSmHudSupers (cheats.h), config.ini `modern_missile`.
int ModernControls_Missile(void);
void ModernControls_SetMissile(int item);

// The remappable part (docs/modern-controls.md, "Remapping"). Four actions sit on the four face
// buttons, a permutation; three weapons, held with R, each ride on one action's button, and the
// missile is never on the run action: R + run is the shortcut of another weapon, so missiles are not fired while running (R + run + shoot is not practical to press). Weapons follow their action when the actions
// move. The defaults are the scheme as it always was: A jump, B run, X shoot, Y morph ball, and
// R + X missile, R + B X-ray, R + Y grapple.
typedef enum { kModernJump, kModernRun, kModernShoot, kModernMorph, kModernActions } ModernAction;
typedef enum { kModernMissile, kModernXray, kModernGrapple, kModernWeapons } ModernWeapon;
typedef enum { kFaceA, kFaceB, kFaceX, kFaceY, kFaceButtons } ModernFace;

// The face button an action is on, the action a weapon rides on, and the weapon an action carries
// (-1 for none).
ModernFace ModernControls_ButtonOf(ModernAction action);
ModernAction ModernControls_WeaponAction(ModernWeapon weapon);
int ModernControls_ActionWeapon(ModernAction action);
// The button a weapon is held with (R + that one): its action's.
ModernFace ModernControls_WeaponButton(ModernWeapon weapon);

// Puts an action on a button; the action that was there takes its old one. The weapons stay with
// their actions, so moving actions never breaks the missile rule.
void ModernControls_SetButton(ModernAction action, ModernFace face);
// Puts a weapon on the action that has `face`; a weapon already there trades places with it.
// Returns false (nothing changes) for the missile on the run action.
bool ModernControls_SetWeaponButton(ModernWeapon weapon, ModernFace face);
void ModernControls_ResetMapping(void);
// The whole table in one number (to tell whether it changed), and from the config.ini names.
unsigned ModernControls_MappingKey(void);
const char *ModernControls_FaceName(ModernFace face);         // "A", "B", "X", "Y"
const char *ModernControls_ActionName(ModernAction action);   // "jump", "run", "shoot", "morph"
bool ModernControls_FaceFromName(const char *name, ModernFace *out);
bool ModernControls_ActionFromName(const char *name, ModernAction *out);

// `inputs` (the frontend's pad bits, SNES order from bit 0: B Y SELECT START Up Down Left Right
// A X L R) as the game must see them this frame. With the original controls it returns them as
// they are; with the modern ones, during gameplay, it translates them and asks the game for the
// item the buttons want. Called once a game frame, before it runs.
int ModernControls_Translate(int inputs);
