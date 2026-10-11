#include "sram_writer.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define kMaxSram 0x2000

static Thread g_thread;
static bool g_thread_failed;
static LightEvent g_wake;
static LightLock g_lock;            // guards everything below
static uint8_t g_pending_buf[kMaxSram];
static size_t g_pending_size;
static bool g_pending, g_busy;
static bool g_result_ready;
static float g_result_ms;

// The new file is written whole beside the old one and only then put in its place, so there is
// always a complete file on the card: sm.srm, or sm.srm.tmp (written, not yet renamed), or
// sm.srm.bak (the save before). RtlReadSram tries them in that order.
static bool WriteFile(const uint8_t *data, size_t size) {
  FILE *f = fopen("saves/sm.srm.tmp", "wb");
  if (!f) return false;
  bool ok = fwrite(data, 1, size, f) == size;
  ok = fflush(f) == 0 && ok;
  ok = fclose(f) == 0 && ok;
  if (!ok) {
    remove("saves/sm.srm.tmp");
    return false;
  }
  remove("saves/sm.srm.bak");
  rename("saves/sm.srm", "saves/sm.srm.bak");   // may not exist yet
  return rename("saves/sm.srm.tmp", "saves/sm.srm") == 0;
}

static void WorkerMain(void *arg) {
  (void)arg;
  static uint8_t local[kMaxSram];
  for (;;) {
    LightEvent_Wait(&g_wake);
    for (;;) {
      LightLock_Lock(&g_lock);
      if (!g_pending) {
        g_busy = false;
        LightLock_Unlock(&g_lock);
        break;
      }
      const size_t size = g_pending_size;
      memcpy(local, g_pending_buf, size);
      g_pending = false;
      g_busy = true;
      LightLock_Unlock(&g_lock);

      const u64 t0 = osGetTime();
      const bool ok = WriteFile(local, size);
      const float ms = (float)(osGetTime() - t0);
      if (!ok) fprintf(stderr, "Unable to write saves/sm.srm\n");

      LightLock_Lock(&g_lock);
      g_result_ready = true;
      g_result_ms = ok ? ms : -ms;
      LightLock_Unlock(&g_lock);
    }
  }
}

// Started on the first save, so a session without one never has the thread.
static bool StartThread(void) {
  if (g_thread) return true;
  if (g_thread_failed) return false;
  LightLock_Init(&g_lock);
  LightEvent_Init(&g_wake, RESET_ONESHOT);
  s32 prio = 0x30;
  svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
  // One step below the game: it only needs the CPU while the game waits for the vblank, and most of
  // its time is spent waiting for the file system anyway. Same core as the game (-1 = the default).
  g_thread = threadCreate(WorkerMain, NULL, 16 * 1024, prio + 1, -1, false);
  if (!g_thread) g_thread_failed = true;
  return g_thread != NULL;
}

void SramWriter_Queue(const uint8_t *data, size_t size) {
  if (size > kMaxSram) size = kMaxSram;
  if (!StartThread()) {   // no thread: write it here, as before
    const u64 t0 = osGetTime();
    const bool ok = WriteFile(data, size);
    g_result_ms = ok ? (float)(osGetTime() - t0) : -(float)(osGetTime() - t0);
    g_result_ready = true;
    return;
  }
  LightLock_Lock(&g_lock);
  memcpy(g_pending_buf, data, size);
  g_pending_size = size;
  g_pending = true;
  LightLock_Unlock(&g_lock);
  LightEvent_Signal(&g_wake);
}

void SramWriter_Flush(void) {
  if (!g_thread) return;
  for (;;) {
    LightLock_Lock(&g_lock);
    const bool idle = !g_pending && !g_busy;
    LightLock_Unlock(&g_lock);
    if (idle) return;
    svcSleepThread(1000000);
  }
}

bool SramWriter_TakeResult(float *ms) {
  if (!g_thread && !g_result_ready) return false;
  if (g_thread) LightLock_Lock(&g_lock);
  const bool ready = g_result_ready;
  if (ready) {
    *ms = g_result_ms;
    g_result_ready = false;
  }
  if (g_thread) LightLock_Unlock(&g_lock);
  return ready;
}
