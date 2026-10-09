#ifndef Z80_H
#define Z80_H

#define LSB_FIRST

#define INT_RST00   0x00C7
#define INT_RST08   0x00CF
#define INT_RST10   0x00D7
#define INT_RST18   0x00DF
#define INT_RST20   0x00E7
#define INT_RST28   0x00EF
#define INT_RST30   0x00F7
#define INT_RST38   0x00FF
#define INT_IRQ     INT_RST38
#define INT_NMI     0xFFFD
#define INT_NONE    0xFFFF
#define INT_QUIT    0xFFFE

#define S_FLAG      0x80
#define Z_FLAG      0x40
#define H_FLAG      0x10
#define P_FLAG      0x04
#define V_FLAG      0x04
#define N_FLAG      0x02
#define C_FLAG      0x01

#define IFF_1       0x01
#define IFF_IM1     0x02
#define IFF_IM2     0x04
#define IFF_2       0x08
#define IFF_EI      0x20
#define IFF_HALT    0x80

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

#ifndef WORD_TYPE_DEFINED
#define WORD_TYPE_DEFINED
typedef unsigned short word;
#endif

typedef signed char offset;

typedef union
{
  struct { byte l, h; } B;
  word W;
} pair;



// Estrutura alinhada a 16 bytes (128 bits - barramento nativo da Emotion Engine)
typedef struct __attribute__((aligned(16)))
{
  pair AF, BC, DE, HL, IX, IY, PC, SP;
  pair AF1, BC1, DE1, HL1;
  byte IFF, I;
  byte R;

  int IPeriod, ICount;
  int IBackup;
  word IRequest;
  byte IAutoReset;
  byte TrapBadOps;
  word Trap;
  byte Trace;
  void *User;
} Z80;

void ResetZ80(Z80 * __restrict__ R);
word ExecZ80(Z80 * __restrict__ R);
void IntZ80(Z80 * __restrict__ R, word Vector);
word RunZ80(Z80 * __restrict__ R);

void WrZ80(word Addr, byte Value);
byte RdZ80(word Addr);
void OutZ80(word Port, byte Value);
byte InZ80(word Port);
void PatchZ80(Z80 *R);
word LoopZ80(Z80 *R);

#endif /* Z80_H */