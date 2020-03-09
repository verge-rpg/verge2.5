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

#if !defined(INTERINC)
#define INTERINC

#ifdef __WATCOMC__
  #include <i86.h>
  #define REGISTERS union REGS
  #define REG_AX(r) (r).w.ax
  #define REG_BX(r) (r).w.bx
  #define REG_CX(r) (r).w.cx
  #define REG_DX(r) (r).w.dx
  #define INTERRUPT(i,r) int386((i),&(r),&(r))
#elif defined(__DJGPP__)
  #include <dpmi.h>
  #define REGISTERS __dpmi_regs
  #define REG_AX(r) (r).x.ax
  #define REG_BX(r) (r).x.bx
  #define REG_CX(r) (r).x.cx
  #define REG_DX(r) (r).x.dx
  #define INTERRUPT(i,r) __dpmi_int((i),&(r))
//  #define inp inportb
  #define outp outportb
  #define outpw outportw
#endif // watcom | djgpp

#endif // INTERINC
