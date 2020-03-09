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
// Wee. A generic #include. I feel so warm and fuzzy inside. :)

// #include "mss.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <aen, may 5>
// + added VERSION tag and made all text occurences of version use it
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef VERGE_INC
#define VERGE_INC

#define VERSION "2.5 beta 5"

#ifdef _WIN32
  #include <windows.h>
  #define delay Sleep
#endif

#ifndef _WIN32
  #include <dos.h>
#endif

extern "C" {
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include "mikmod.h"
}

#define byte unsigned char
#define word unsigned short int
#define quad unsigned int

#define TRUE 1
#define FALSE 0

extern void Sys_Error(char* message, ...);

#include "linked.h"

#include "inter.h"

#include "a_memory.h"

#include "rawmem.h"
#include "rawstr.h"

#include "console.h"
#include "controls.h"
#include "engine.h"
#include "entity.h"
#ifndef _WIN32
#include "fli.h"
#endif
#include "font.h"
#include "image.h"
#include "keyboard.h"
#include "main.h"
#include "message.h"
#include "mouse.h"

#include "render.h"
#include "sound.h"
#include "timer.h"
#include "vfile.h"
#include "vc.h"
#include "vdriver.h"

extern int hicolor;

extern char *strbuf;

extern void V_memset (void *dest, int fill, int count);
extern void V_memcpy (void *dest, void *src, int count);
extern int  V_memcmp (void *m1, void *m2, int count);
extern void V_strcpy (char *dest, char *src);
extern void V_strncpy(char* dest, char* src, int count);
extern int  V_strlen (char *str);
extern void V_strcat (char *dest, char *src);
extern int  V_strcmp (char* s1, char* s2);
extern int V_atoi (char *str);
extern float V_atof (char *str);

/*
void CD_Init();
void CD_Deinit();
void CD_Open_Door();
void CD_Close_Door();
void CD_Stop();
void CD_Play(int track);
*/

#endif // VERGE_INC
