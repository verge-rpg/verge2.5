/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
// ³                          The VERGE Engine                           ³
// ³              Copyright (C)1998 BJ Eirich (aka vecna)                ³
// ³                    Timer / PIC contoller module                     ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#define TIMER_H
#include <conio.h>
#include "verge.h"

#define PIT0 0x40
#define PIT1 0x41
#define PIT2 0x42
#define PITMODE 0x43
#define PITCONST 1193180L

#define OCR1    0x20
#define IMR1    0x21

#define OCR2    0xA0
#define IMR2    0xA1

// ================================= Data ====================================

void (__interrupt __far *biosTimerHandler)();
char timerinstalled=0;

unsigned int systemtime=0, timer_count=0, hktimer=0, vctimer=0;
int (*callback) (void);

// ================================= Code ====================================

// #define disable() __asm { cli }
// #define enable() __asm { sti }

/*
static SendEOI (unsigned char irqno)
{
  static unsigned char ocr;
  static unsigned char eoi;

         ocr=(irqno>7) ? OCR2 : OCR1;
         eoi=0x60|(irqno&7);
         outp(ocr,eoi);
         if (irqno>7) outp(OCR1,0x20);
}
*/

extern int thresh;

void __interrupt __far newhandler(void)
{
	systemtime++;
	timer_count++;
	hktimer++;
	vctimer++;

	if (cpu_watch)	CPUTick();
	CheckTileAnimation();
	//if (callback)	callback();
	MD_Update();

	outp(0x20,0x20);
}

void SetHZ(unsigned int hz)
{ unsigned int pit0_set, pit0_value;

  _disable();

  outp(PITMODE, 0x34);
  pit0_value=PITCONST / hz;
  pit0_set=(pit0_value & 0x00ff);
  outp(PIT0, pit0_set);
  pit0_set=(pit0_value >> 8);
  outp(PIT0, pit0_set);

  _enable();
}

void RestoreHZ()
{
  _disable();
  outp(PITMODE, 0x34);
  outp(PIT0, 0x00);
  outp(PIT0, 0x00);
  _enable();
}

void InitTimer()
{
  if (timerinstalled) return;

  biosTimerHandler=_dos_getvect(0x08);
  _dos_setvect(0x08, newhandler);
  timerinstalled=1;
  SetHZ(100);
}

void ShutdownTimer()
{
  if (timerinstalled)
  {
    _dos_setvect(0x08, biosTimerHandler);
    RestoreHZ();
    timerinstalled=0;
  }
}
