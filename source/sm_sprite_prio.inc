/* Rooms whose sprites are drawn under the walls the owner sent to a nearer stereo plane (sm_plane_fixes.inc).
 * Kept apart from that file because tools/layer-workbench rewrites it whole.
 *
 * SM_SPRITE_CAP(room, cap)
 *     A sprite of an OAM priority above `cap` is drawn as one of priority `cap` in that room (GPU renderer, WIDE
 *     gameplay): a cap of 2 puts every sprite under the BG1 tiles of priority 1, which a fix can give to the walls.
 *     Without it a priority 3 sprite (a boss) is drawn over the walls while the 3D shows it behind them.
 *
 *   0xCD13 Phantoon's room: Phantoon and his fireballs are priority 3, over the frame, which is nearer in 3D.
 */

SM_SPRITE_CAP(0xCD13, 2)
