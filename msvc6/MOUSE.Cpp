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

/*
    mouse.c
    coded by aen
    ---
    last updated: 30oct99
*/

#include "inter.h"

static int mouse_x				= 0;
static int mouse_y				= 0;
static int mouse_button_flags	= 0;
static int mouse_restrict		= 1;

extern int screen_width;
extern int screen_length;

//static rect_t cage;	// coming soon

void Mouse_Init()
{
	REGISTERS r;

	REG_AX(r) = 0;
	INTERRUPT(0x33,r);
	if (!REG_AX(r))
	{
		// mouse not present
	}
}

void Mouse_Read()
{
	REGISTERS r;

// get mickeys
	REG_AX(r) = 0x0b;
	INTERRUPT(0x33, r);

	mouse_x += (signed short) REG_CX(r);
	mouse_y += (signed short) REG_DX(r);

	if (mouse_restrict)
	{
		if (mouse_x < 0)
			mouse_x = 0;
		else if (mouse_x >= screen_width)
		{
			mouse_x = screen_width - 1;
		}
		if (mouse_y < 0)
			mouse_y = 0;
		else if (mouse_y >= screen_length)
		{
			mouse_y = screen_length - 1;
		}
	}

// get button status
	REG_AX(r) = 0x03;
	INTERRUPT(0x33, r);
	mouse_button_flags = r.w.bx;
}

void Mouse_SetPosition(int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}

int Mouse_X()
{
	return mouse_x;
}

int Mouse_Y()
{
	return mouse_y;
}

int Mouse_Button(int button)
{
	return mouse_button_flags & (1 << button);
}

int Mouse_ButtonFlags()
{
	return mouse_button_flags;
}

void Mouse_ButtonSetFlags(int flags)
{
	mouse_button_flags = flags;
}
