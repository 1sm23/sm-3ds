/* Mode 7 cells that go to another stereo plane than the Mode 7 plane's own (PLAY, or the room's SM_LAYER_PLANE for
 * layer 5). Written by hand: the layer workbench does not show Mode 7. At most two planes per room besides the plane's
 * own (the renderer decodes each into a 1024x1024 texture of its own).
 *
 * SM_M7_CELLS(room, x0, y0, x1, y1, plane)
 *     The cells (x0, y0)..(x1, y1) of the plane's map (0..127) go to `plane`. Checked first.
 * SM_M7_TILES(room, first, last, plane)
 *     The cells showing tiles first..last (the tile numbers of the plane's map, 0..255) go to `plane` (StereoPlane,
 *     stereo_depth.h). Checked after SM_M7_CELLS, before SM_M7_SOLID; the first rule that matches wins.
 * SM_M7_SOLID(room, plane)
 *     The cells over a solid block of the room's level data (type 8) go to `plane`, for a plane drawn 1:1 with the room
 *     (cell (cx, cy) on block (cx / 2, cy / 2)): the scenery Samus collides with.
 * SM_M7_FILL(room, reach)
 *     Behind a cell sent to a nearer plane, each farther plane's texture shows the nearest cell on that plane straight
 *     above it (else below), at most `reach` cells away, under the nearer cell's own pixels only: the other eye sees past
 *     the nearer cell's edge, where the plane has no picture (it was black).
 * SM_M7_FILL_ACROSS(room, x0, y0, x1, y1)
 *     In the cells (x0, y0)..(x1, y1) the fill comes from the same row instead (left or right): where the picture
 *     above does not continue behind. The eyes shift at most 5 px apart (FRONT to FAR), so one cell across is enough.
 * SM_M7_NO_FILL(room, x0, y0, x1, y1)
 *     No fill behind the cells (x0, y0)..(x1, y1): the farther planes stay empty there. For SM_M7_FILL_ACROSS and
 *     SM_M7_NO_FILL the first area in the file that holds a cell wins.
 *
 *   0xDF45 (the Ceres elevator shaft): the back wall behind the shaft (girders, mesh panels, the dark between them)
 *     goes to FAR; the side walls, the hazard stripes, the columns, the ledges, the elevator, the door and the floor
 *     stay on PLAY. The tiles were picked from the plane's map: those drawn only between the hazard stripes (cells 8-23
 *     of the 32-cell shaft, rows 0-96) and never outside, minus the elevator's. What Samus collides with (the walls, the
 *     ledges, the elevator's platform, the floor and the columns at the bottom) goes to FRONT, with her (her sprites go
 *     there by the SM_LAYER_PLANE of sm_plane_fixes.inc); so does the wall pattern that fills the plane outside the room
 *     (tiles 98-9B, seen in WIDE's margins), which has no level data under it, and the whole elevator (its lower corners
 *     are not over solid blocks).
 */

SM_M7_TILES(0xDF45, 0x42, 0x49, kStereoFront)
SM_M7_TILES(0xDF45, 0x4E, 0x4F, kStereoFront)
SM_M7_TILES(0xDF45, 0x52, 0x59, kStereoFront)
SM_M7_TILES(0xDF45, 0x5E, 0x5F, kStereoFront)
SM_M7_TILES(0xDF45, 0x68, 0x69, kStereoFront)
SM_M7_TILES(0xDF45, 0x6E, 0x6F, kStereoFront)
SM_M7_TILES(0xDF45, 0x78, 0x78, kStereoFront)
SM_M7_TILES(0xDF45, 0x7E, 0x7F, kStereoFront)
SM_M7_TILES(0xDF45, 0x8F, 0x91, kStereoFar)
SM_M7_TILES(0xDF45, 0x98, 0x9B, kStereoFront)
SM_M7_TILES(0xDF45, 0xA0, 0xC3, kStereoFar)
SM_M7_TILES(0xDF45, 0xC5, 0xC8, kStereoFar)
SM_M7_TILES(0xDF45, 0xCB, 0xCE, kStereoFar)
SM_M7_TILES(0xDF45, 0xD0, 0xD3, kStereoFar)
SM_M7_TILES(0xDF45, 0xD5, 0xD8, kStereoFar)
SM_M7_TILES(0xDF45, 0xDB, 0xDE, kStereoFar)
SM_M7_TILES(0xDF45, 0xE0, 0xE5, kStereoFar)
SM_M7_TILES(0xDF45, 0xF5, 0xF5, kStereoFar)
SM_M7_TILES(0xDF45, 0xF7, 0xF8, kStereoFar)
SM_M7_TILES(0xDF45, 0xFF, 0xFF, kStereoFar)
SM_M7_SOLID(0xDF45, kStereoFront)
SM_M7_FILL(0xDF45, 2)                       /* a ledge is two cells high */
SM_M7_FILL_ACROSS(0xDF45, 8, 74, 13, 75)    /* the end of the lowest left ledge (block row 37), over the back wall */
SM_M7_NO_FILL(0xDF45, 12, 11, 19, 15)       /* the elevator: a copy behind it showed as a darker piece */
