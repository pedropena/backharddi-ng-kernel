extern WORD g_wEndian;

#define ENDIAN_UNKNOWN 0
#define ENDIAN_LITTLE  1
#define ENDIAN_BIG     2
#define ENDIAN_PDP     3

// swapping macros
#define swab16(x) \
        ((unsigned short)( \
                (((unsigned short)(x) & (unsigned short)0x00ffU) << 8) | \
                (((unsigned short)(x) & (unsigned short)0xff00U) >> 8) ))

#define swab32(x) \
        ((unsigned int)( \
                (((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) | \
                (((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) | \
                (((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) | \
                (((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))
#define swab64(x) \
        ((unsigned long long)( \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x00000000000000ffULL) << 56) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x000000000000ff00ULL) << 40) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x0000000000ff0000ULL) << 24) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x00000000ff000000ULL) <<  8) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x000000ff00000000ULL) >>  8) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x0000ff0000000000ULL) >> 24) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0x00ff000000000000ULL) >> 40) | \
                (unsigned long long)(((unsigned long long)(x) & (unsigned long long)0xff00000000000000ULL) >> 56) )) 

inline void setEndianess(BOOL bShowPrintf)
{
  DWORD dwNb = 0x11223344;
  BYTE *cBuffer;
  cBuffer = (BYTE*)&dwNb;

  g_wEndian = ENDIAN_UNKNOWN;
  
  if ((cBuffer[0] == 0x44) && (cBuffer[1] == 0x33) && (cBuffer[2] == 0x22) && (cBuffer[3] == 0x11)) // little endian
    {
      g_wEndian = ENDIAN_LITTLE;
      if (bShowPrintf) printf ("Processor = ENDIAN_LITTLE\n");
    }
  
  if ((cBuffer[0] == 0x11) && (cBuffer[1] == 0x22) && (cBuffer[2] == 0x33) && (cBuffer[3] == 0x44)) // big endian
    {
      g_wEndian = ENDIAN_BIG;
      if (bShowPrintf) printf ("Processor = ENDIAN_BIG\n");
    }

  if ((cBuffer[0] == 0x22) && (cBuffer[1] == 0x11) && (cBuffer[2] == 0x44) && (cBuffer[3] == 0x33)) // pdp endian
    {
      g_wEndian = ENDIAN_PDP;
      if (bShowPrintf) printf ("Processor = ENDIAN_PDP\n");
    }
}

// ------------- CPU TO XXX --------------
#define CpuToLe16(a) ((g_wEndian == ENDIAN_LITTLE) ? a : swab16(a))
#define CpuToBe16(a) ((g_wEndian == ENDIAN_BIG) ? a : swab16(a))
#define CpuToLe32(a) ((g_wEndian == ENDIAN_LITTLE) ? a : swab32(a))
#define CpuToBe32(a) ((g_wEndian == ENDIAN_BIG) ? a : swab32(a))
#define CpuToLe64(a) ((g_wEndian == ENDIAN_LITTLE) ? a : swab64(a))
#define CpuToBe64(a) ((g_wEndian == ENDIAN_BIG) ? a : swab64(a))

#define CpuToLe(a) \
    ((sizeof(a) == 8) ? CpuToLe64(a) : \
    ((sizeof(a) == 4) ? CpuToLe32(a) : \
    ((sizeof(a) == 2) ? CpuToLe16(a) : \
    (a))))

#define CpuToBe(a) \
    ((sizeof(a) == 8) ? CpuToBe64(a) : \
    ((sizeof(a) == 4) ? CpuToBe32(a) : \
    ((sizeof(a) == 2) ? CpuToBe16(a) : \
    (a))))

// ------------- XXX TO CPU --------------
#define Le16ToCpu(a) CpuToLe16(a)
#define Le32ToCpu(a) CpuToLe32(a)
#define Le64ToCpu(a) CpuToLe64(a)

#define Be16ToCpu(a) CpuToBe16(a)
#define Be32ToCpu(a) CpuToBe32(a)
#define Be64ToCpu(a) CpuToBe64(a)

#define LeToCpu(a) \
    ((sizeof(a) == 8) ? Le64ToCpu(a) : \
    ((sizeof(a) == 4) ? Le32ToCpu(a) : \
    ((sizeof(a) == 2) ? Le16ToCpu(a) : \
    (a))))

#define BeToCpu(a) \
    ((sizeof(a) == 8) ? Be64ToCpu(a) : \
    ((sizeof(a) == 4) ? Be32ToCpu(a) : \
    ((sizeof(a) == 2) ? Be16ToCpu(a) : \
    (a))))


