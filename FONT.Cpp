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
// ³                 Font Loading / Text Output module                   ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

/*
	mod log:

	<aen>
	30	December	1999	Major revamp.
*/

#include "verge.h"

// DATA ////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_FONTS 10

struct fontbay_t
{
	string	filename;

	int		cell_width, cell_length;	// font cell dimensions
	int		font_sets;
	int		active_set;

	rawmem	font_data;
};

static fontbay_t font_bay[10];			// Font stuff;
static int font_x, font_y, font_alignx;	// x/y font location

// CODE ////////////////////////////////////////////////////////////////////////////////////////////

unsigned char* Font_GetPtr(int slot) { return font_bay[slot].font_data.get(0, 1); }

static void Font_CheckSlot(int slot)
{
	if (slot<0 || slot >= MAX_FONTS)
		Sys_Error("Font_CheckSlot: bogus slot request (reported %d).\n", slot);
}

int Font_GetWidth(int slot)
{
	Font_CheckSlot(slot);

	return font_bay[slot].cell_width;
}

int Font_GetLength(int slot)
{
	Font_CheckSlot(slot);

	return font_bay[slot].cell_length;
}

/*
static int Font_GetSubsets(int slot)
{
	Font_CheckSlot(slot);

	return font_bay[slot].font_sets;
}

static int Font_GetActiveSubset(int slot)
{
	Font_CheckSlot(slot);

	return font_bay[slot].active_set;
}
*/

static void Font_SetActiveSubset(int slot, int subset)
{
	Font_CheckSlot(slot);

	if (subset < 0 || subset >= font_bay[slot].font_sets)
	{
		Log(va("Font_SetActiveSubset: %s: bogus subset request (reported %d).",
			font_bay[slot].filename.c_str(), subset));
	}

	font_bay[slot].active_set = subset;
}

static int Font_SlotInUse(int slot)
{
	Font_CheckSlot(slot);

	return (font_bay[slot].font_data.length() > 0);
}

static int Font_FindFreeSlot()
{
	int		n;

	for (n = 0; n < MAX_FONTS; n += 1)
	{
		if (!Font_SlotInUse(n))
			return n;
	}

	Sys_Error("LoadFont: no free slots found (%d slots max).\n", MAX_FONTS);
	return 0;
}

/*static int Font_FreeSlots()
{
	int count=0;
	for (int n=0; n<MAX_FONTS; n++)
	{
		if (Font_SlotInUse(n))
			count++;
	}
	return count;
}*/

/*static int Font_UsedSlots()
{
	return MAX_FONTS-Font_FreeSlots();
}*/

static void LoadBiosFont(int slot)
{
	int		n, mask, count;
	char*	bf;
	char*	ptr;

	bf = (char *) 0xffa6e + (8*32);

	font_bay[slot].cell_width	= 8;
	font_bay[slot].cell_length	= 8;
	font_bay[slot].font_sets	= 1;
	font_bay[slot].active_set	= 0;
	font_bay[slot].font_data.resize(8*8*96);

	ptr = (char *)font_bay[slot].font_data.get(0, 8*8*96);

	for (n = 0; n < 96; n += 1)
	{
		mask = 0x80;
		count = 8*8;
		do
		{
			if (*bf & mask)
				*ptr = 31;
			else
				*ptr = 0;

			ptr += 1;
			mask >>= 1;
			if (!mask)
			{
				mask = 0x80;
				bf += 1;
			}
			count -= 1;
		}
		while (count);
	}
}

int Font_Load(string filename)
{
	int		need;
	int		ver;
	char*	zstr;
	VFILE*	f;
	const int	slot = Font_FindFreeSlot();

	// set filename
	font_bay[slot].filename = filename;

	zstr = filename.c_str();
	f = vopen(zstr);
	if (!f)
	{
		Log(va("LoadFont: %s: unable to open, loading default font (BIOS 8x8)", zstr));
		LoadBiosFont(slot);

		return slot;
	}

	ver = vgetc(f);
	if (ver != 1)
	{
		vclose(f);
		Sys_Error("LoadFont: %s: incorrect version number (reported %d).\n", zstr, ver);
	}

	// set font dims
	font_bay[slot].cell_width	= vgetw(f);
	if (font_bay[slot].cell_width < 1 || font_bay[slot].cell_width > 128)
	{
		vclose(f);
		Sys_Error("LoadFont: %s: bogus cell-width (reported %d).\n", zstr, font_bay[slot].cell_width);
	}
	font_bay[slot].cell_length	= vgetw(f);
	if (font_bay[slot].cell_length < 1 || font_bay[slot].cell_length > 128)
	{
		vclose(f);
		Sys_Error("LoadFont: %s: bogus cell-length (reported %d).\n", zstr, font_bay[slot].cell_length);
	}

	// set subsets
	font_bay[slot].font_sets	= vgetw(f);
	font_bay[slot].active_set	= 0;
	if (font_bay[slot].font_sets < 0 || font_bay[slot].font_sets > 4)
	{
		vclose(f);
		Sys_Error("LoadFont: %s: bogus set count (reported %d).\n", zstr, font_bay[slot].font_sets);
	}

	need = font_bay[slot].font_sets*96*(font_bay[slot].cell_width*font_bay[slot].cell_length);
	font_bay[slot].font_data.resize(need);
	vread(font_bay[slot].font_data.get(0, need), need, f);

	if (hicolor)
	{
		unsigned char* old=new unsigned char [need];
		if (!old)
			Sys_Error("Font_Load: memory exhausted on hicolor:old");
		memcpy(old, font_bay[slot].font_data.get(0, 1), need);

		font_bay[slot].font_data.resize(need*2);
		unsigned short *hip = (unsigned short *)font_bay[slot].font_data.get(0,1);
		for (int n=0; n<need; n++)
		{
			if (!old[n])
				hip[n] = (16 == hicolor) ? 0xF81F : 0x7C1F;
			else
				hip[n] = LoToHi(old[n]);
		}
		delete[] old;
	}

	vclose(f);

	return slot;
}

void Font_Unload(int slot)
{
	if (Font_SlotInUse(slot))
	{
		font_bay[slot].filename		= "";
		font_bay[slot].cell_width	= 0;
		font_bay[slot].cell_length	= 0;
		font_bay[slot].font_sets	= 0;
		font_bay[slot].active_set	= 0;
		font_bay[slot].font_data.resize(0);
	}
}

void Font_GotoXY(int x, int y)
{
	font_x = font_alignx = x;
	font_y = y;
}

int Font_GetX() { return font_x; }
int Font_GetY() { return font_y; }

static int _char_slot = 0;

// assumes _char_slot is valid
static void Font_PrintChar(char ch)
{
	int		width;
	int		length;
	int		offset;
	unsigned char*	ptr;

	if (ch < 32 && ch >= 127)
		return;

	width	= font_bay[_char_slot].cell_width;
	length	= font_bay[_char_slot].cell_length;

	ch -= 32;	// convert to font bay character offset
	offset	= (font_bay[_char_slot].active_set*96*width*length) + (ch*width*length);
	ptr	= font_bay[_char_slot].font_data.get(offset*(hicolor?2:1), width*length);

	LFB_Blit(font_x, font_y, width, length, ptr, 1, 0);
	//TintClip(font_x, font_y, width, length, 31, ptr);
}

void Font_Print(int slot, string str)
{
	char	ch;
	char*	zstr;

	Font_CheckSlot(slot);
	_char_slot = slot;

	zstr = str.c_str();
	while (*zstr)
	{
		ch = *zstr++;
		switch (ch)
		{
			case 126: Font_SetActiveSubset(slot, 0); continue;
			case 128: Font_SetActiveSubset(slot, 1); continue;
			case 129: Font_SetActiveSubset(slot, 2); continue;
			case 130: Font_SetActiveSubset(slot, 3); continue;
		}

		Font_PrintChar(ch);
		font_x += font_bay[slot].cell_width;
	}
}

void Font_PrintImbed(int slot, string str)
{
	char	ch;
	char*	zstr;

	Font_CheckSlot(slot);
	_char_slot = slot;

	zstr = str.c_str();
	while (*zstr)
	{
		ch = *zstr++;
		switch (ch)
		{
			case 126: Font_SetActiveSubset(slot, 0); continue;
			case 128: Font_SetActiveSubset(slot, 1); continue;
			case 129: Font_SetActiveSubset(slot, 2); continue;
			case 130: Font_SetActiveSubset(slot, 3); continue;

			case '\t':
			{
				// font_alignx should always be >= font_x
				int chx = (font_x - font_alignx)/font_bay[slot].cell_width;
				font_x += (4 - (chx % 4))*font_bay[slot].cell_width;
				continue;
			}
			case '\n':
				font_y += font_bay[slot].cell_length;
				font_x  = font_alignx;
				continue;
		}

		Font_PrintChar(ch);
		font_x += font_bay[slot].cell_width;
	}
}