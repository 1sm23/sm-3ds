# Plan: extending non-gameplay screens in WIDE / PIXEL PERFECT with generated art

**Status: planned, nothing built (2026-10-14).** Task P4.19 in `PLAN.md`. This file is the
whole brief: when the owner says "let's build it", start at "Order of work". Only the items
marked **VERIFY** need a console dump first.

Same idea as the Minish Cap and Zero Mission ports, adapted to this game. References, in
the owner's other checkouts:

- `../zelda-tmc-3ds/docs/wide-scene-extension.md` and `tools/scene-extend/`: the working
  implementation (dump, template, AI prompt, check, converter, presenter).
- `../mzm/docs/3ds-wide-scene-extension-plan.md`: the same plan for Zero Mission (scene
  recipes A-G and the AI delivery package are defined there; this file reuses them).

## 1. What and why

With WIDE on, gameplay shows more of the room: 72 SNES pixels a side and 8 rows above and
below in PIXEL PERFECT, so the 256x224 frame sits in the 400x240 top screen
(`docs/PLAN.md` P4.5, `source/sm_wide.c`). Every other state keeps the plain frame: black
sides (PIXEL PERFECT) or the 274x240 scaled picture with black sides (SCALED). The owner
wants those screens to fill 400x240 as well. The game has no data past its 256x224 frame, so
the missing parts are **generated with an image AI from a screen dump** and drawn by the
port around the game's own picture.

Differences from the Zelda case that make this one **smaller**:

- Only **72 px per side and 8 rows** are missing (Zelda: 80 and 40). A SCALED frame is 274
  wide, so SCALED needs ~63 px a side instead.
- The game's frame is **opaque** (the SNES PPU's output is a full picture, not layers with
  holes), so the extension can be drawn first, full 400x240, and the game's frame over it.
  No transparency mask is needed for the original area.
- Non-gameplay frames are already **flat in 3D** (`StereoFrame.gameplay` false in
  `gpu_ppu_3ds.c`), so the extension needs no depth plane: same plane as the frame, zero
  parallax. (The Zelda/MZM ports had to think about this; this port does not.)

Scope: `source/` and `tools/` only. **Nothing in `sm/`** is touched (it is the vendored
snesrev game; the PC build and the native-vs-ROM comparison must stay as they are).

## 2. What exists today (verified 2026-10-14)

- `SmWide_*` and `RoomShown()` in `source/sm_wide.c` decide when the margins show the room:
  only gameplay states (7, 8, 12, 18, 27, 42, Ceres escape's end 32-33) and door
  transitions. All other `game_state` values draw the plain 256x224.
- `GpuPpu3ds_DrawAndPresent(frame, pixel_perfect, slider, gameplay, screen)` in
  `source/gpu_ppu_3ds.c` draws the frame (GPU targets 256x256, 512x256 when WIDE); the CPU
  path (`DrawPpuFrame` in `source/main.c`) draws refused frames. Both would need the
  extension: GPU first, CPU path as the fallback.
- Dumps (`docs/debug-tools.md`): `sm-dump-NNNN-top.rgb` (PPU output, headerless RGB8,
  **256x240**, check which rows are the 224 visible ones), `-vram.bin`, `-cgram.bin`,
  `-oam.bin`, `-highoam.bin`, `-ppu.txt` (register state), `-obj.txt`, `-game.txt`
  (game state), `-wram.bin`. Take them with `tools/sm-link/sm-link.py` (state, load a save
  state, `shot`, a dump from the bottom-screen DEBUG tools). Fetch by file name over FTP
  (port 5000); never list a folder with hundreds of dumps.
- The scene recorder (SCENE REC, `tools/scene-rec/decode.py`) finds the frame to dump in
  cutscenes.
- `tools/gpu-ppu-test` host harness (`WIDE=M`, `WIDE_DUMP=N`) renders frames on the PC from
  save states: handy to produce a layer-isolated picture without the console.
- Release notes, settings text and strings: `docs/translations.md` rules apply to any new
  UI string (every `romfs/lang/*.txt` and `docs/lang-template.txt`).

## 3. Scene inventory (SM states)

`game_state` values are `kGameState_*` in `sm/src/ida_types.h`. Every recipe cell is a
**hypothesis**: confirm with a dump (**VERIFY**) before spending an AI run. Recipes A-G are
defined in `../mzm/docs/3ds-wide-scene-extension-plan.md` section 5: A fixed picture, B
plain colour (stretch the edge), C tall scrolling BG (strips), D wide BG (extend the
ends), E affine/windows/HDMA (leave ORIGINAL), F palette-cycled (indices), G procedural.

| Screen | States | Probable recipe |
|---|---|---|
| Title, opening cinematic (space, planet, text pages) | 0, 1 | **A** per page, or **B** where it is black; text boxes are BG3, not painted. **VERIFY** which layers hold the planet and whether it scrolls (C) |
| Game options menu | 2 | **B** or **A** |
| File select, file copy/clear, file select map | 4, 5 | **A** (the screen is menus on one backdrop); the map screen may be **E** |
| Pause map and equipment screens | 12-18 (pausing, paused) | menus over a backdrop: **A**/**B**. Note 12 and 18 are in `RoomShown()` today (the darkening); the pause screens themselves are not. **VERIFY** |
| Death sequence, game over menu | 19-26 | **A** or **B** (mostly black with the sprite effects) |
| Ceres escape, time up, Ceres exploding | 32-38 | Ceres is **mode 7**: the plane is 1024x1024, so a wider view may show real plane data instead of generated art (**VERIFY** in the PPU's mode 7 path). The rest: **A**/**B** |
| Ending and credits (Samus pictures, the credits' scrolling text) | 39 | **A** per picture; credits text scrolls (**C**-like, mostly black). The best ratio of effort to result |
| Demo transitions | 41, 43, 44 | **B**; demos themselves (42) already show the room |

## 4. Port work, in order

1. **Scene gate** (new `source/sm_scene_view.[ch]`; leave `sm_wide.c`'s gameplay logic
   alone): `SmScene_Active()` when WIDE is on, PIXEL PERFECT (decide SCALED, section 7),
   and `game_state` is a non-gameplay state; the scene id from `game_state` plus
   whatever page/cutscene variable the screen has.
2. **Frame placement**: the top screen presents the 256x224 frame at (72, 8) in a 400x240
   canvas in those states. No game-side change.
3. **Edge stretch (recipe B)** for every non-gameplay state, first: the frame's outer
   columns and rows stretched to the screen's edges, no assets. This alone removes the
   black bars on menus and black cutscenes.
4. **Picture loader**: a file per scene, from the SD card first (`sdmc:/3ds/Super Metroid
   3DS/wide-scenes/`, an override) then the romfs (`romfs:/wide-scenes/`, shipped), lazy,
   freed on leaving the scene; RGB565 or palette indices; the game's own colour handling.
   Copy the shape of `../zelda-tmc-3ds/platform/3ds/source/scene_pictures_3ds.c` but with
   a small descriptor table instead of one global per scene.
5. **Presenter**: draw the picture (400x240, whole: the frame goes over it) before the
   game's frame, in the GPU path in `gpu_ppu_3ds.c` and in the CPU fallback in `main.c`.
6. **Fades**: the game fades with INIDISP brightness and colour math. The extension must
   fade with it (Zelda: `VeilScenePicture`; here derive the level from the same registers
   the renderer already reads for the frame). **VERIFY** on the opening cinematic and the
   ending, which fade a lot.
7. **Palette animation** (recipe F): where a backdrop shimmers by palette rotation, ship
   indices and rebuild from the frame's CGRAM, as the Zelda file select does.
8. **Debug and settings**: the WIDE option's text in OPTIONS should say what it now
   does; any new string goes through `docs/translations.md`.
9. **Test**: with WIDE off nothing changes (`make test` hashes), and the host harness
   `tools/gpu-ppu-test` gets a case that renders each scene state at 400x240 and checks the
   middle 256x224 equals the plain frame.

## 5. Tooling work

Copy `tools/scene-extend/` from the Zelda checkout into `tools/scene-extend/` here, in
English, and adapt:

- **Input**: SNES, not GBA. Backgrounds are 2/4/8 bpp planar tiles, 32x32 tilemaps
  (up to 64x64 by `BGnSC`), 8x8 or 16x16 tiles, flip and priority bits in the tilemap
  word; tile and map bases come from `-ppu.txt` (`BG12NBA`, `BGnSC`, `BGMODE`). CGRAM is
  `-cgram.bin` (256 x BGR555). Mode 7 is separate (1024x1024 plane of 8x8 8bpp tiles).
- Simplest source for the **composite** is `-top.rgb`. For the **background alone**, decode
  the layer from VRAM with the dump's registers, or render it with the host harness
  (`tools/gpu-ppu-test`) with the other layers off.
- `check_extension.py`: unchanged (0 changed pixels in the original area, 0 off-palette).
- `to_console.py`: scene table for SM (name -> size, file name, mode: whole / indices),
  output `wide-scenes/<scene>.bin`, upload to the data folder over the game's FTP server
  (`tools/sm-link`'s transport, the credentials in `source/ftp_credentials.h`).
- `--package` writes the AI delivery folder of section 6 in one command.

## 6. The AI delivery package (one folder per scene)

Identical in spirit to `../mzm/docs/3ds-wide-scene-extension-plan.md` section 8b. Folder
`~/Imágenes/sm-<scene>/` **outside the repository, always** (it is the ROM's art):

| File | Content |
|---|---|
| `README-AI.md` | The task, exact numbers, rules (template below) |
| `1-template-400x240.png` | The canvas: the 256x224 original at x=72..327, y=8..231, transparent elsewhere |
| `2-original-picture.png` | The game's background alone, 256x224, 1:1, PNG, no resampling |
| `3-final-composite.png` | The screen as the player sees it, context only |
| `4-do-not-paint.png` | Optional: the text boxes, sprites and effects drawn over the background |

SCALED needs a 274x240 canvas with the 256x224 frame scaled 1.071 in the middle; do not
generate for it unless the owner asks (section 7): the original would have to be resampled.

```markdown
# Extend a Super Metroid (SNES) background: <scene name>

Pixel-art background from Super Metroid (256x224, 1 pixel = 1 game pixel). Enlarge it to
400x240 by generating the missing area around it: 72 px left, 72 px right, 8 px above,
8 px below.

## Files
- `1-template-400x240.png`: the canvas. The original is at x=72..327, y=8..231; everything
  else is transparent and is what you fill.
- `2-original-picture.png`: the original alone.
- `3-final-composite.png`, `4-do-not-paint.png`: context only.

## What continues at each edge
<one sentence per side>

## Rules
1. Do not change a single pixel inside the original rectangle.
2. Use only colours that already appear in the original (<N> colours). No anti-aliasing,
   gradients, blur, dithering the original does not use, or new hues.
3. Same pixel scale and style: hard 1-pixel edges, the same outlines and shading.
4. Continue what touches the original's edges so the seams are invisible; more of the same
   kind of element, nothing new. Do not repeat or mirror the original.
5. Do not paint what `4-do-not-paint.png` shows: <list>. Background only.
6. Output: a PNG exactly 400x240, no transparency, no scaling or smoothing, <2-3>
   variants as separate files.
```

The output is never trusted: `check_extension.py` against `2-original-picture.png` must
say 0 changed pixels (otherwise paste the original back over it) and 0 off-palette.

## 7. Order of work

1. Port the tooling (section 5). One console session: dump every state of section 3
   (title, each opening page, file select, ending pictures, game over, Ceres) with
   `sm-link`; fill the **VERIFY** cells.
2. Scene gate, frame placement and edge stretch (recipe B) for all non-gameplay states.
3. **Pilot: the title screen**, end to end with an AI picture: loader, presenter, fades.
4. Ending pictures, opening cinematic pages, file select, game over.
5. Mode 7 (Ceres): check first whether a wider view can read real plane data; only then
   consider art.
6. Release notes line per shipped batch (`docs/release-notes/<tag>.md`); ask before a minor
   bump.

Done when, per screen: the owner sees it at 400x240 on the console in WIDE + PIXEL
PERFECT, no seam at the frame's edges, fades and palette animation continue into the
extension, with no file the sides are plain (black or edge-stretched) and never garbage,
and WIDE off is bit-identical (`make test`).

## 8. Open decisions for the owner

- ~~Ship generated parts or keep them on the SD card?~~ **Decided 2026-10-14 (owner): ship
  them in the romfs**, so players get the screens with no setup (as the Zelda port does).
  Only the generated areas ship, the original area left transparent: no pixel of the game's
  art is in the files, and no ROM data is ever in the package (the CI's refusal of a
  `.smc`/`.sfc` in `romfs/` is unaffected). A file of the same name in the SD data folder
  still overrides the shipped one. Dumps, templates, whole pictures and the AI delivery
  folders stay out of the repository.
- **SCALED**: skip (PIXEL PERFECT only) or generate a second set at 274x240.
- **Which screens** merit an AI run; the rest keep edge stretch.
- **Mode 7**: show more plane natively, or leave Ceres as it is.
- **Old 3DS cost**: one extra full-screen textured quad per frame on these screens; check
  with the PERF recorder after the pilot.
