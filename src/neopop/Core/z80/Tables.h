#ifndef TABLES_H
#define TABLES_H

#include "Z80.h"

// Alinhamento de 64 bytes = Tamanho exato da linha da L1 Data Cache do PS2
#define PS2_CACHE_ALIGN __attribute__((aligned(64)))

static const byte Cycles[256] PS2_CACHE_ALIGN =
{
   4,10, 7, 6, 4, 4, 7, 4, 4,11, 7, 6, 4, 4, 7, 4,
   8,10, 7, 6, 4, 4, 7, 4,12,11, 7, 6, 4, 4, 7, 4,
   7,10,16, 6, 4, 4, 7, 4, 7,11,16, 6, 4, 4, 7, 4,
   7,10,13, 6,11,11,10, 4, 7,11,13, 6, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
   5,10,10,10,10,11, 7,11, 5,10,10, 0,10,17, 7,11,
   5,10,10,11,10,11, 7,11, 5, 4,10,11,10, 0, 7,11,
   5,10,10,19,10,11, 7,11, 5, 4,10, 4,10, 0, 7,11,
   5,10,10, 4,10,11, 7,11, 5, 6,10, 4,10, 0, 7,11 
};

static const byte CyclesCB[256] PS2_CACHE_ALIGN =
{
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
   8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
   8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
   8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
   8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8 
};

static const byte CyclesED[256] PS2_CACHE_ALIGN =
{
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  12,12,15,20, 8,14, 8, 9,12,12,15,20, 0,14, 0, 9,
  12,12,15,20, 0, 0, 8, 9,12,12,15,20, 0, 0, 8, 9,
  12,12,15,20, 0, 0, 0,18,12,12,15,20, 0, 0, 0,18,
  12, 0,15,20, 0, 0, 0, 0,12,12,15,20, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  16,16,16,16, 0, 0, 0, 0,16,16,16,16, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const byte CyclesXX[256] PS2_CACHE_ALIGN =
{
   0, 0, 0, 0, 0, 0, 0, 0, 0,15, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0,15, 0, 0, 0, 0, 0, 0,
   0,14,20,10, 9, 9, 9, 0, 0,15,20,10, 9, 9, 9, 0,
   0, 0, 0, 0,23,23,19, 0, 0,15, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 9, 9,19, 0, 0, 0, 0, 0, 9, 9,19, 0,
   0, 0, 0, 0, 9, 9,19, 0, 0, 0, 0, 0, 9, 9,19, 0,
   9, 9, 9, 9, 9, 9,19, 9, 9, 9, 9, 9, 9, 9,19, 9,
  19,19,19,19,19,19,19,19, 0, 0, 0, 0, 9, 9,19, 0,
   0, 0, 0, 0, 9, 9,19, 0, 0, 0, 0, 0, 9, 9,19, 0,
   0, 0, 0, 0, 9, 9,19, 0, 0, 0, 0, 0, 9, 9,19, 0,
   0, 0, 0, 0, 9, 9,19, 0, 0, 0, 0, 0, 9, 9,19, 0,
   0, 0, 0, 0, 9, 9,19, 0, 0, 0, 0, 0, 9, 9,19, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0,14, 0,23, 0,15, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0, 0, 0, 0, 0, 0
};

static const byte CyclesXXCB[256] PS2_CACHE_ALIGN =
{
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0,
   0, 0, 0, 0, 0, 0,23, 0, 0, 0, 0, 0, 0, 0,23, 0
};

static const byte ZSTable[256] PS2_CACHE_ALIGN =
{
  Z_FLAG,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,
  S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG,S_FLAG
};

static const byte PZSTable[256] PS2_CACHE_ALIGN =
{
  Z_FLAG|P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,
  0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,
  0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,
  0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,
  P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,
  0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,
  P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,
  P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,
  0,P_FLAG,P_FLAG,0,P_FLAG,0,0,P_FLAG,P_FLAG,0,0,P_FLAG,0,P_FLAG,P_FLAG,0,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG,S_FLAG|P_FLAG,S_FLAG|P_FLAG,S_FLAG,
  S_FLAG|P_FLAG,S_FLAG,S_FLAG,S_FLAG|P_FLAG
};

#endif /* TABLES_H */