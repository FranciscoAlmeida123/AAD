//
// Tomás Oliveira e Silva,  October 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// CRC computation (PNG version)
//

#ifndef AAD_PNG_CRC
#define AAD_PNG_CRC

//
// png_crc_table[]
//
// the Cyclic Redundancy Check (CRC) computation is the remainder of one polynomial by another polynomial (the CRC polynomial)
// all computations are done modulo 2, so addition is an XOR and multiplication is an AND
//
// the CRC polynomial is
//   x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
// the corresponding bit numbers for the PNG implementation of the CRC are
//     -1      5      8      9     15     19     20     21    23    24    26    27    29  30  31
// that is, the least signficant bit corresponds to the coefficient of the x^31 term
//
// multiplication by x corresponds to a right shift; if the shifted out bit is one (that is the x^32 coefficient), we need to
// subtract the CRC polynomial (apply an xor with the bit mask of the CRC polynomial coefficients)
//
// the CRC is linear: if cA is the CRC of A and cB is the CRC of B then the CRC of (A xor B) is (cA xor cB)
// this is explored in the initialization of the png_crc_table[] array; using linearity, this array allows the computation of the CRC 8 bits at a time
//
// explanation of the constants that follow:
//   0xEDB88320u = the CRC polynomial
//   0x76DC4190u =  0xEDB88320u >> 1                 (multiplication by x)
//   0x3B6E20C8u =  0x76DC4190u >> 1                 (multiplication by x)
//   0x1DB71064u =  0x3B6E20C8u >> 1                 (multiplication by x)
//   0x0EDB8832u =  0x1DB71064u >> 1                 (multiplication by x)
//   0x076DC419u =  0x0EDB8832u >> 1                 (multiplication by x)
//   0x076DC419u =  0x0EDB8832u >> 1                 (multiplication by x)
//   0xEE0E612Cu = (0x076DC419u >> 1) ^ 0xEDB88320u  (multiplication by x)
//   0x77073096u =  0xEE0E612Cu >> 1                 (multiplication by x)
//

#define PNG_CRC_POLYNOMIAL 0xEDB88320u // 0b_1110_1101_1011_1000_1000_0011_0010_0000

#define CRC_BIT_7    CRC_BIT_6(0),CRC_BIT_6(    0xEDB88320u)
#define CRC_BIT_6(x) CRC_BIT_5(x),CRC_BIT_5(x ^ 0x76DC4190u)
#define CRC_BIT_5(x) CRC_BIT_4(x),CRC_BIT_4(x ^ 0x3B6E20C8u)
#define CRC_BIT_4(x) CRC_BIT_3(x),CRC_BIT_3(x ^ 0x1DB71064u)
#define CRC_BIT_3(x) CRC_BIT_2(x),CRC_BIT_2(x ^ 0x0EDB8832u)
#define CRC_BIT_2(x) CRC_BIT_1(x),CRC_BIT_1(x ^ 0x076DC419u)
#define CRC_BIT_1(x) CRC_BIT_0(x),CRC_BIT_0(x ^ 0xEE0E612Cu)
#define CRC_BIT_0(x)           x ,          x ^ 0x77073096u

static const unsigned int png_crc_table[256] = { CRC_BIT_7 };

#undef CRC_BIT_7
#undef CRC_BIT_6
#undef CRC_BIT_5
#undef CRC_BIT_4
#undef CRC_BIT_3
#undef CRC_BIT_2
#undef CRC_BIT_1
#undef CRC_BIT_0

static unsigned int update_png_crc(unsigned int crc,const char *buffer,int length)
{
  for(int n = 0;n < length;n++)
    crc = png_crc_table[(crc ^ (unsigned int)buffer[n]) & 0xFFu] ^ (crc >> 8);
    // this is the same as
    // 1. crc = png_crc_table[crc & 0xFFu] ^ (crc >> 8); --- multiplication by x^8
    // 2. crc = crc ^ png_crc_table[buffer[n]]           --- add the contribution of the next byte
    // remember, the CRC computation is linear (so, the table contents are also linear), thus we can reorder the computation so that only one table lookup is necessary
  return crc;
}

static unsigned int compute_png_crc(const char *buffer,int length)
{
  return update_png_crc(0xFFFFFFFFu,buffer,length) ^ 0xFFFFFFFFu;
}

#endif
