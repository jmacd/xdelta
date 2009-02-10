// 9 is a common value for cksum_size, but the exact value shouldn't
// matter.
const usize_t CKSUM_SIZE = 9;

// These values are filled-in by FindCksumCollision
uint8_t golden_cksum_bytes[CKSUM_SIZE] = {
  0x8d, 0x83, 0xe7, 0x6f, 0x46, 0x7f, 0xed, 0x51, 0xe6 
};
uint8_t collision_cksum_bytes[CKSUM_SIZE] = {
  0xaf, 0x55, 0x16, 0x89, 0x7c, 0x70, 0x00, 0xe5, 0xa7
};

void FindCksumCollision() {
  // TODO! This is not being used.
  if (golden_cksum_bytes[0] != 0) {
    CHECK(memcmp(golden_cksum_bytes, collision_cksum_bytes, CKSUM_SIZE) != 0);
    CHECK_EQ(xd3_lcksum(golden_cksum_bytes, CKSUM_SIZE),
	     xd3_lcksum(collision_cksum_bytes, CKSUM_SIZE));
    return;
  }

  MTRandom rand;
  MTRandom8 rand8(&rand);

  for (size_t i = 0; i < CKSUM_SIZE; i++) {
    collision_cksum_bytes[i] = golden_cksum_bytes[i] = rand8.Rand8();
  }

  uint32_t golden = xd3_lcksum(golden_cksum_bytes, CKSUM_SIZE);

  size_t i = 0;
  while (true) {
    collision_cksum_bytes[i++] = rand8.Rand8();

    if (golden == xd3_lcksum(collision_cksum_bytes, CKSUM_SIZE) &&
	memcmp(collision_cksum_bytes, golden_cksum_bytes, CKSUM_SIZE) != 0) {
      break;
    }

    if (i == CKSUM_SIZE) {
      i = 0;
    }
  }

  Block b1, b2;
  b1.Append(golden_cksum_bytes, CKSUM_SIZE);
  b2.Append(collision_cksum_bytes, CKSUM_SIZE);
  
  DP(RINT "Found a cksum collision\n");
  b1.Print();
  b2.Print();
}
