int rijndaelSetupDecrypt(u32 *rk, const u8 *key, int keybits)
{
  int nrounds, i, j;
  u32 temp;

  /* expand the cipher key: */
  nrounds = rijndaelSetupEncrypt(rk, key, keybits);
  /* invert the order of the round keys: */
  for (i = 0, j = 4*nrounds; i < j; i += 4, j -= 4)
  {
    temp = rk[i    ]; rk[i    ] = rk[j    ]; rk[j    ] = temp;
    temp = rk[i + 1]; rk[i + 1] = rk[j + 1]; rk[j + 1] = temp;
    temp = rk[i + 2]; rk[i + 2] = rk[j + 2]; rk[j + 2] = temp;
    temp = rk[i + 3]; rk[i + 3] = rk[j + 3]; rk[j + 3] = temp;
  }
  /* apply the inverse MixColumn transform to all round keys but the first and the last: */
  for (i = 1; i < nrounds; i++)
  {
    rk += 4;
    rk[0] =
      Td0[Te4[(rk[0] >> 24)       ] & 0xff] ^
      Td1[Te4[(rk[0] >> 16) & 0xff] & 0xff] ^
      Td2[Te4[(rk[0] >>  8) & 0xff] & 0xff] ^
      Td3[Te4[(rk[0]      ) & 0xff] & 0xff];
    rk[1] =
      Td0[Te4[(rk[1] >> 24)       ] & 0xff] ^
      Td1[Te4[(rk[1] >> 16) & 0xff] & 0xff] ^
      Td2[Te4[(rk[1] >>  8) & 0xff] & 0xff] ^
      Td3[Te4[(rk[1]      ) & 0xff] & 0xff];
    rk[2] =
      Td0[Te4[(rk[2] >> 24)       ] & 0xff] ^
      Td1[Te4[(rk[2] >> 16) & 0xff] & 0xff] ^
      Td2[Te4[(rk[2] >>  8) & 0xff] & 0xff] ^
      Td3[Te4[(rk[2]      ) & 0xff] & 0xff];
    rk[3] =
      Td0[Te4[(rk[3] >> 24)       ] & 0xff] ^
      Td1[Te4[(rk[3] >> 16) & 0xff] & 0xff] ^
      Td2[Te4[(rk[3] >>  8) & 0xff] & 0xff] ^
      Td3[Te4[(rk[3]      ) & 0xff] & 0xff];
  }
  return nrounds;
}