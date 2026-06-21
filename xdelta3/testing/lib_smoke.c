/* Consumer smoke test for the installed/embedded xdelta3 library.
   Links against the public xd3_* API only (xdelta3.h) and performs a
   round-trip encode/decode of an in-memory buffer against a source. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xdelta3.h"

int main(void) {
  static uint8_t source[4096];
  static uint8_t target[4096];
  static uint8_t delta[8192];
  static uint8_t output[4096];
  usize_t delta_size = 0;
  usize_t output_size = 0;
  int ret;

  for (usize_t i = 0; i < sizeof(source); i += 1) {
    source[i] = (uint8_t)(i * 7 + 3);
    target[i] = source[i];
  }
  /* Diverge the target so the delta is non-trivial. */
  for (usize_t i = 1000; i < 1100; i += 1) {
    target[i] = (uint8_t)(255 - i);
  }

  ret = xd3_encode_memory(target, sizeof(target), source, sizeof(source), delta,
                          &delta_size, sizeof(delta), 0);
  if (ret != 0) {
    fprintf(stderr, "xd3_encode_memory failed: %d\n", ret);
    return 1;
  }

  ret = xd3_decode_memory(delta, delta_size, source, sizeof(source), output,
                          &output_size, sizeof(output), 0);
  if (ret != 0) {
    fprintf(stderr, "xd3_decode_memory failed: %d\n", ret);
    return 1;
  }

  if (output_size != sizeof(target) ||
      memcmp(output, target, sizeof(target)) != 0) {
    fprintf(stderr, "round-trip mismatch (output_size=%u)\n",
            (unsigned)output_size);
    return 1;
  }

  printf("xdelta3 library smoke test passed (delta %u bytes)\n",
         (unsigned)delta_size);
  return 0;
}
