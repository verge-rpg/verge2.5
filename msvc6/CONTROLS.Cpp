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
// ³                       Controls system module                        ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#include <conio.h>
#include "verge.h"

// ================================= Data ====================================

// -- Hardware Dependent -- //

int kb1, kb2, kb3, kb4;             // keyboard controls
int jb1, jb2, jb3, jb4;             // joystick controls

// -- Unified Interface -- //

int b1, b2, b3, b4;
int up=0, down=0, left=0, right=0;

// -- UnPress support variables -- //

/*
char wb1, wb2, wb3, wb4;
char wup, wdown, wleft, wright;

char rb1, rb2, rb3, rb4;
char rup, rdown, rleft, rright;
*/

// ================================= Code ====================================

int foundx,foundy;
int jx,jy;

void GetCoordinates()
{
  foundx=0;
  foundy=0;

  __asm
  {
    cli
    mov dx, 513
    out dx, al
    xor ecx, ecx
    mov ebx, 2
joyloop:
    inc ecx
    cmp ecx, 65500
    jae j_end
    in  al,dx
    cmp foundx, 1
    je search_y

    test al, 1
    jnz j0
    mov jx, ecx
    mov foundx, 1
    dec ebx
    jz  j_end
j0:
    cmp foundy, 1
    je joyloop
search_y:
    test al, 2
    jnz joyloop
    mov jy, ecx
    mov foundy, 1
    dec ebx
    jz  j_end
    jmp joyloop
j_end:
    sti
  }
}

int cenx,ceny,upb,leftb,rightb,downb;

int Calibrate()
{ // assumes the stick is centered when called.

  GetCoordinates();                  // read stick position
  if ((!foundx) || (!foundy))
     { printf("Could not detect joystick. Disabling.\n");
       return 0; }

  cenx=jx;
  ceny=jy;
  upb=(ceny*75)/100;                 // 25% dead zone
  leftb=(cenx*75)/100;
  rightb=(cenx*125)/100;             // 25% dead zone
  downb=(ceny*125)/100;
  return 1;
}

void ReadButtons()
{
  int b;
  int btbl[4];

  b=inp(0x201);                       // poll joystick port
  b=b >> 4;                           // lose high nibble
  b=b ^ 15;                           // flip mask bits

  btbl[0]=b & 1;                      // mask button status
  btbl[1]=b & 2;
  btbl[2]=b & 4;
  btbl[3]=b & 8;

  /*
  if (btbl[jb1-1]) rb1=1;
  if (btbl[jb2-1]) rb2=1;
  if (btbl[jb3-1]) rb3=1;
  if (btbl[jb4-1]) rb4=1;
  */
}

ReadJoystick()
{
  ReadButtons();
  GetCoordinates();

  /*
  if (jx<leftb) rleft=1;
  if (jx>rightb) rright=1;
  if (jy<upb) rup=1;
  if (jy>downb) rdown=1;
  */
}

void Key_Game(int key)
{
	if (key=='`')
	{
		key_dest=key_console;
		Console_Activate();
		key_dest=key_game;
	}

	if (key == K_F11)
	{
		Message_Send("Screen snapshot taken.", 50);
		ScreenShot();
	}
}

void UpdateControls()
{
	Key_SendKeys();

/*
//  rb1=0; rb2=0; rb3=0; rb4=0;
//  rup=0; rdown=0; rleft=0; rright=0;

	if (foundx && foundy) ReadJoystick();

	b1=key_down[kb1];
	b2=key_down[kb2];
	b3=key_down[kb3];
	b4=key_dowm[kb4];

	up=key_down[SCAN_UP];
	down=key_down[SCAN_DOWN];
	right=key_down[SCAN_RIGHT];
	left=key_down[SCAN_LEFT];
*/

/*
  if (key_down[SCAN_UP]) rup=1;
  if (key_down[SCAN_DOWN]) rdown=1;
  if (key_down[SCAN_RIGHT]) rright=1;
  if (key_down[SCAN_LEFT]) rleft=1;

  if (key_down[kb1]) rb1=1;
  if (key_down[kb2]) rb2=1;
  if (key_down[kb3]) rb3=1;
  if (key_down[kb4]) rb4=1;

  if (foundx && foundy) ReadJoystick();

       if (wb1 && rb1) rb1=0;
  else if (wb1 && !rb1) wb1=0;
  b1=rb1;

       if (wb2 && rb2) rb2=0;
  else if (wb2 && !rb2) wb2=0;
  b2=rb2;

       if (wb3 && rb3) rb3=0;
  else if (wb3 && !rb3) wb3=0;
  b3=rb3;

       if (wb4 && rb4) rb4=0;
  else if (wb4 && !rb4) wb4=0;
  b4=rb4;

       if (wup && rup) rup=0;
  else if (wup && !rup) wup=0;
  up=rup;

       if (wdown && rdown) rdown=0;
  else if (wdown && !rdown) wdown=0;
  down=rdown;

       if (wleft && rleft) rleft=0;
  else if (wleft && !rleft) wleft=0;
  left=rleft;

       if (wright && rright) rright=0;
  else if (wright && !rright) wright=0;
  right=rright;

	if (console_cheat())
	{
		Message("Console access granted.",500);
		Con_SetOverride(1);
	}

	CheckHookTimer();
	*/
}

void UnPress(int control)
{
	switch (control)
	{
		case 0:
			if (key_down[kb1]) { unpress[kb1]=1; b1=0; }
			if (key_down[kb2]) { unpress[kb2]=1; b2=0; }
			if (key_down[kb3]) { unpress[kb3]=1; b3=0; }
			if (key_down[kb4]) { unpress[kb4]=1; b4=0; }
			break;
		case 1:
			if (key_down[kb1]) { unpress[kb1]=1; b1=0; }
			break;
		case 2:
			if (key_down[kb2]) { unpress[kb2]=1; b2=0; }
			break;
		case 3:
			if (key_down[kb3]) { unpress[kb3]=1; b3=0; }
			break;
		case 4:
			if (key_down[kb4]) { unpress[kb4]=1; b4=0; }
			break;
		case 5:
			if (key_down[K_UP]) { unpress[K_UP]=1; up=0; }
			break;
		case 6:
			if (key_down[K_DOWN]) { unpress[K_DOWN]=1; down=0; }
			break;
		case 7:
			if (key_down[K_LEFT]) { unpress[K_LEFT]=1; left=0; }
			break;
		case 8:
			if (key_down[K_RIGHT]) { unpress[K_RIGHT]=1; right=0; }
			break;
	}
}