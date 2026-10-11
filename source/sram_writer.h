// The game's battery RAM (saves/sm.srm) written off the main thread.
//
// The SD card makes the write (rename of the old file, create, 8 KB, close) take far longer than
// a frame, and the game used to wait for it in the middle of the save station's animation. The
// copy of the SRAM is taken at the call; a worker thread writes it while the game goes on. The
// SRAM in memory is the one the game reads back, so nothing waits for the file.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Takes a copy of `data` and has it written to saves/sm.srm. A write still pending is replaced.
void SramWriter_Queue(const uint8_t *data, size_t size);

// Blocks until nothing is pending or being written. Called before the process exits.
void SramWriter_Flush(void);

// Main thread, once a frame: true once per finished write, with how long the SD card took (ms).
bool SramWriter_TakeResult(float *ms);
