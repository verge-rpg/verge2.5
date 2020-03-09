
#ifndef __VIRTCH_C__
#define __VIRTCH_C__

#include "mikmod.h"

// ** For PC-assembly mixing
// =========================
// Uncomment both lines below for assembly mixing under WATCOM or GCC for
// Linux.  Note that there is no 16 bit mixers for assembly yet (C only), so
// defining __ASSEMBLY__ when not defining __FASTMIXER__ will lead to compiler
// errors.

#define __FASTMIXER__
#define __ASSEMBLY__

// Various other VIRTCH.C Compiler Options
// =======================================

// BITSHIFT : Controls the maximum volume of the sound output.  All data
//      is shifted right by BITSHIFT after being mixed.  Higher values
//      result in quieter sound and less chance of distortion.  If you
//      are using the assembly mixer, you must change the bitshift const-
//      ant in RESAMPLE.ASM or RESAMPLE.S as well!

#define BITSHIFT 9

// BOUNDS_CHECKING : Forces VIRTCH to perform bounds checking.  Commenting
//      the line below will result in a slightly faster mixing process but
//      could cause nasty clicks and pops on some modules.  Disable this
//      option on games or demos only, where speed is very important all
//      songs / sndfx played can be specifically tested for pops.

#define BOUNDS_CHECKING


typedef struct
{   UBYTE  kick;                  // =1 -> sample has to be restarted
    UBYTE  active;                // =1 -> sample is playing
    UWORD  flags;                 // 16/8 bits looping/one-shot
    SWORD  handle;                // identifies the sample
    ULONG  start;                 // start index
    ULONG  size;                  // samplesize
    ULONG  reppos;                // loop start
    ULONG  repend;                // loop end
    ULONG  frq;                   // current frequency
    int    vol;                   // current volume
    int    pan;                   // current panning position

    UWORD  volramp;
    int    lvol, rvol,
           loldvol, roldvol,
           lvolinc, rvolinc;

    SLONGLONG  current;           // current index in the sample
    SLONGLONG  increment;         // fixed-point increment value
} VINFO;

#ifdef __GNUC__
#define __cdecl
#endif

#ifdef __WATCOMC__
#define   inline
#endif

#define FRACBITS 32
#define FRACMASK (SLONGLONG)((1UL<<FRACBITS)-1UL)

#define INTERPBITS 16
#define INTERPMASK (SLONG)((1ul<<INTERPBITS)-1ul)


#define TICKLSIZE 8192
#define TICKWSIZE (TICKLSIZE*2)
#define TICKBSIZE (TICKWSIZE*2)

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

// Volume tables and selectors for 8 and 16 bit mixing.

extern SLONG  **voltab;
extern SLONG   *lvoltab, *rvoltab;
extern SLONG  lvolsel, rvolsel;   


extern void Mix8StereoNormal(SBYTE *srce, SLONG *dest, SLONG *index_hi, SLONGLONG *index, SLONGLONG increment, SLONG todo);
extern void Mix8StereoInterp(SBYTE *srce, SLONG *dest, SLONG *index_hi, ULONG *index_lo, SLONGLONG *index, SLONGLONG increment, SLONG todo);
extern void Mix8SurroundNormal(SBYTE *srce, SLONG *dest, SLONG *index_hi, SLONGLONG *index, SLONGLONG increment, SLONG todo);
extern void Mix8SurroundInterp(SBYTE *srce, SLONG *dest, SLONG *index_hi, ULONG *index_lo, SLONGLONG *index, SLONGLONG increment, SLONG todo);
extern void Mix8MonoNormal(SBYTE *srce, SLONG *dest, SLONG *index_hi, SLONGLONG *index, SLONGLONG increment, SLONG todo);
extern void Mix8MonoInterp(SBYTE *srce, SLONG *dest, SLONG *index_hi, ULONG *index_lo, SLONGLONG *index, SLONGLONG increment, SLONG todo);

extern void Mix8StereoNormal_NoClick(SBYTE *srce, SLONG *dest, SLONG *index_hi,SLONGLONG *index, SLONGLONG increment, SLONG todo, int lvol, int rvol, int lvolinc, int rvolinc);
extern void Mix8StereoInterp_NoClick(SBYTE *srce, SLONG *dest, SLONG *index_hi, ULONG *index_lo, SLONGLONG *index, SLONGLONG increment, SLONG todo, int lvol, int rvol, int lvolinc, int rvolinc);
extern void Mix8SurroundNormal_NoClick(SBYTE *srce, SLONG *dest, SLONG *index_hi, SLONGLONG *index, SLONGLONG increment, SLONG todo, int vol, int volinc);
extern void Mix8SurroundInterp_NoClick(SBYTE *srce, SLONG *dest, SLONG *index_hi, ULONG *index_lo, SLONGLONG *index, SLONGLONG increment, SLONG todo, int vol, int volinc);
extern void Mix8MonoNormal_NoClick(SBYTE *srce, SLONG *dest, SLONG *index_hi, SLONGLONG *index, SLONGLONG increment, SLONG todo, int vol, int volinc);
extern void Mix8MonoInterp_NoClick(SBYTE *srce, SLONG *dest, SLONG *index_hi, ULONG *index_lo, SLONGLONG *index, SLONGLONG increment, SLONG todo, int vol, int volinc);

#endif
