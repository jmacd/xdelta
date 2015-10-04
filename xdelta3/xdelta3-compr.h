int gz_detect_func(uint8_t *data, int len)
{
  uint8_t flags;
  if(len < 9) return -1;
  flags = data[8];
  switch(flags) {
    case 2: return 9; /* maximum compr */
    case 4: return 1; /* minimum compr */
  }
  return 6; /* could be 2..8 but 6 is default */
}

int bz2_detect_func(uint8_t *data, int len)
{
  if(len < 4) return -1;
  return data[3]&0xf;
}

int xz_detect_func(uint8_t *data, int len)
{
  int offs;
  int dict_size;
  /* there might be extra headers which need to be skipped */
  for( offs = 14; offs < 26; offs++) {
    if(offs+2 >= len) return -1;
    if(data[offs+0] != 0x21) continue; /* LZMA filter */
    if(data[offs+1] != 0x01) continue; /* Size of Filter Properties:  1 byte */
    dict_size = data[offs+2];
    switch(dict_size) {
      case 12: return 0;
      case 16: return 1;
      case 18: return 2;
      case 20: return 3; /* could also be 4 */
      case 22: return 6; /* could also be 5 but 6 is the default so the guess is correct in 99% of cases */
      case 24: return 7;
      case 26: return 8;
      case 28: return 9;
      default: return -1; /* not guessable */
    }
  }
  return -1;
}
