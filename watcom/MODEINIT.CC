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
// ³                         Mode Init module                            ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

/*
	modifications:

	<aen>
	January 22, 2000

		Revamp.
*/

#include <conio.h>
#include "verge.h"

#include "mouse.h"

#include "vbe2.h"

int tmask;
int trans_mask;

void BIOS_SetVideoMode(int mode)
{
	REGISTERS r;
	REG_AX(r) = (unsigned short) mode;
	INTERRUPT(0x10, r);
}

// assumes game_palette components run in the 0..63 range
unsigned short LoToHi(int index)
{
	unsigned char*	p;

	if (index < 0)
		index = 0;
	if (index > 255)
		index = 255;

	p = game_palette + 3*index;

	return (16 == hicolor)
		? (unsigned short)( ((p[0]>>1)<<11) | (p[1]<<5) | (p[2]>>1) )
		: (unsigned short)( ((p[0]>>1)<<10) | ((p[1]>>1)<<5) | (p[2]>>1) );
}

unsigned short _24ToHi(int r, int g, int b)
{
	return (16 == hicolor)
		? (unsigned short)( ((r>>3)<<11) | ((g>>2)<<5) | (b>>3) )
		: (unsigned short)( ((r>>3)<<10) | ((g>>3)<<5) | (b>>3) );
}

// =============================== Mode 13h ===================================

int Mode13hShutdown(int text_mode)
{
	if (text_mode)
		BIOS_SetVideoMode(0x3);

	/*
	if (vscreen)
		vfree(vscreen);
	vscreen = 0;
	screen = 0;
	*/

	return 0;
}

void InitMode13h(void)
{
	BIOS_SetVideoMode(0x13);

	DriverDesc = "320x200 (Mode 13h, linear)";

	// setup function drivers
	ShowPage = LFB_ShowPage;
	ShutdownVideo = Mode13hShutdown;

	screen_width = 320;
	screen_length = 200;
}

// ================================= Mode-X ==================================

#define SEQU_ADDR       0x3c4
#define CRTC_ADDR       0x3d4
#define MISC_OUTPUT     0x3c2

#define ATTRCON_ADDR    0x3c0
#define MISC_ADDR       0x3c2
#define VGAENABLE_ADDR  0x3c3
#define SEQ_ADDR        0x3c4
#define GRACON_ADDR     0x3ce
#define CRTC_ADDR       0x3d4
#define STATUS_ADDR     0x3da

struct Register
{
	unsigned int	port;
	unsigned char	index, value;
};

Register scr256x256[] =
{
	{ 0x3c2, 0x00, 0xe3}, { 0x3d4, 0x00, 0x5f}, { 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40}, { 0x3d4, 0x03, 0x82}, { 0x3d4, 0x04, 0x4A},
	{ 0x3d4, 0x05, 0x9A}, { 0x3d4, 0x06, 0x23}, { 0x3d4, 0x07, 0xb2},
	{ 0x3d4, 0x08, 0x00}, { 0x3d4, 0x09, 0x61}, { 0x3d4, 0x10, 0x0a},
	{ 0x3d4, 0x11, 0xac}, { 0x3d4, 0x12, 0xff}, { 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40}, { 0x3d4, 0x15, 0x07}, { 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3}, { 0x3c4, 0x01, 0x01}, { 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40}, { 0x3ce, 0x06, 0x05}, { 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

// ================================= Code ====================================

void ModeX_SetPlane(int plane)
{
	outpw(0x03c4, ((1 << plane) << 8) | 0x02);
}

int ModeX_Shutdown(int text_mode)
{
	if (text_mode)
		BIOS_SetVideoMode(0x3);

	vfree(vscreen);
	vscreen = 0;
	screen = 0;

	return 0;
}

#pragma off (unreferenced);

int ModeX_ShowPage()
{
	unsigned char*	source;
	unsigned char*	dest;
	int		x;
	int		y;
	int		plane;
	const screen_width_div_4 = screen_width >> 2;

	RenderGUI();

	cpubyte = PFLIP;

	dest = video;
	source = screen;
	y = screen_length;

	if (vsync)
	{
		while (!(inp(0x3da) & 8));
	}

// 4 OUTs per row
	do
	{
		for (plane = 0; plane < 4; plane += 1)
		{
			ModeX_SetPlane(plane);

			x = screen_width_div_4;
			do
			{
				*dest = *source;

				source += 4;
				dest += 1;
				x -= 1;
			} while (x);

			source -= (screen_width - 1);
			dest -= screen_width_div_4;
		}
		dest += screen_width_div_4;
		source += screen_width - 4;

		y -= 1;
	} while (y);

	cpubyte = ETC;

	CheckHookTimer();

	return 0;
}

static void outReg(Register r)
{
	switch (r.port)
	{
	// First handle special cases:

	case ATTRCON_ADDR:
		// reset read/write flip-flop
		inp(STATUS_ADDR);
		// ensure VGA output is enabled
		outp(ATTRCON_ADDR, r.index | 0x20);
		outp(ATTRCON_ADDR, r.value);
		break;

	case MISC_ADDR:
	case VGAENABLE_ADDR:
		//  directly to the port
		outp(r.port, r.value);
		break;

	case SEQ_ADDR:
	case GRACON_ADDR:
	case CRTC_ADDR:

	default:
		// index to port
		outp(r.port, r.index);
		// value to port+1
		outp(r.port + 1, r.value);
		break;
	}
}

// readyVgaRegs() does the initialization to make the VGA ready to
// accept any combination of configuration register settings.
//
// This involves enabling writes to index 0 to 7 of the CRT controller
// (port 0x3d4), by clearing the most significant bit (bit 7) of index
// 0x11.

static void readyVgaRegs(void)
{
	int		v;

	outp(0x3d4, 0x11);
	v = inp(0x3d5) & 0x7f;
	outp(0x3d4, 0x11);
	outp(0x3d5, v);
}

// outRegArray sets n registers according to the array pointed to by r.
// First, indexes 0-7 of the CRT controller are enabled for writing.

static void outRegArray(Register* r, int n)
{
	readyVgaRegs();
	while (n)
	{
		outReg(*r);

		r += 1;
		n -= 1;
	}
}

void ModeX_Set256x256()
{
	DriverDesc = "256x256 (ModeX, planar)";

	BIOS_SetVideoMode(0x13);

	outRegArray(scr256x256, sizeof(scr256x256) / sizeof (Register));

	true_screen_width = 256;
	true_screen_length = 256;

	ShowPage = LFB_ShowPage;
	ShutdownVideo = Mode13hShutdown;
}

void ModeX_Set320x240()
{
	int		in_byte;

	DriverDesc = "320x240 (ModeX, planar)";

	BIOS_SetVideoMode(0x13);

	outpw(SEQU_ADDR, 0x0604);
	V_memset((unsigned char *) 0xA0000, 0, 0xffff);
	outpw(SEQU_ADDR, 0x0110);
	outp(MISC_OUTPUT, 0xe3);
	outpw(SEQU_ADDR, 0x0300);
	outp(CRTC_ADDR, 0x11);
	in_byte = inp(CRTC_ADDR + 1);
	in_byte = (in_byte && 0x7f);
	outp((CRTC_ADDR + 1), in_byte);
	outpw(CRTC_ADDR, 0x0d06);
	outpw(CRTC_ADDR, 0x3e07);
	outpw(CRTC_ADDR, 0x4109);
	outpw(CRTC_ADDR, 0xea10);
	outpw(CRTC_ADDR, 0xac11);
	outpw(CRTC_ADDR, 0xdf12);
	outpw(CRTC_ADDR, 0x0014);
	outpw(CRTC_ADDR, 0xe715);
	outpw(CRTC_ADDR, 0x0616);
	outpw(CRTC_ADDR, 0xe317);

	true_screen_width = 320;
	true_screen_length = 240;

	ShowPage = ModeX_ShowPage;
	ShutdownVideo = ModeX_Shutdown;
}

void ModeX_Set360x240()
{
	int		in_byte;

	DriverDesc = "360x240 (ModeX, planar)";

	BIOS_SetVideoMode(0x13);

	outpw(SEQU_ADDR, 0x0604);
	V_memset((unsigned char *) 0xA0000, 0, 0xffff);
	outpw(SEQU_ADDR, 0x100);
	outp(MISC_OUTPUT, 0xe7);
	outpw(SEQU_ADDR, 0x300);
	outp(CRTC_ADDR, 0x11);
	in_byte = inp(CRTC_ADDR + 1);
	in_byte = (in_byte && 0x7f);
	outp((CRTC_ADDR + 1), in_byte);
	outpw(CRTC_ADDR, 0x6b00);
	outpw(CRTC_ADDR, 0x5901);
	outpw(CRTC_ADDR, 0x5a02);
	outpw(CRTC_ADDR, 0x8e03);
	outpw(CRTC_ADDR, 0x5e04);
	outpw(CRTC_ADDR, 0x8a05);
	outpw(CRTC_ADDR, 0x0d06);
	outpw(CRTC_ADDR, 0x3e07);
	outpw(CRTC_ADDR, 0x4109);
	outpw(CRTC_ADDR, 0xea10);
	outpw(CRTC_ADDR, 0xac11);
	outpw(CRTC_ADDR, 0xdf12);
	outpw(CRTC_ADDR, 0x2d13);
	outpw(CRTC_ADDR, 0x0014);
	outpw(CRTC_ADDR, 0xe715);
	outpw(CRTC_ADDR, 0x0616);
	outpw(CRTC_ADDR, 0xe317);

	true_screen_width = 360;
	true_screen_length = 240;

	ShowPage = ModeX_ShowPage;
	ShutdownVideo = ModeX_Shutdown;
}

static int GFX_FindSetMode(int xres, int yres)
{
	if (320 == xres && 200 == yres)
	{
		InitMode13h();
		return 1;
	}
	if (320 == xres && 240 == yres)
	{
		ModeX_Set320x240();
		return 1;
	}
	if (360 == xres && 240 == yres)
	{
		ModeX_Set360x240();
		return 1;
	}
	if (256 == xres && 256 == yres)
	{
		ModeX_Set256x256();
		return 1;
	}

	return 0;
}

static int GFX_FindSetModeVESA(int xres, int yres)
{
	unsigned char*	ptr;
	static char desc[64] = "";

	ptr = (unsigned char *) vbe_set_mode(xres, yres, hicolor ? 16 : 8);
	if (!ptr)
	{
		if (hicolor)
		{
		// 16-bit failed; try 15-bit
			ptr = (unsigned char *) vbe_set_mode(xres, yres, 15);
		}
		if (!ptr)
			return 0; // failure

	// otherwise, if success; hicolor becomes color depth
		if (hicolor)
		{
			hicolor = 15;
			tmask = 0x7BDE;
			trans_mask = 0x7C1F;
		}
	}
	else
	{
		if (hicolor)
		{
			hicolor = 16;
			tmask = 0xF7DE;
			trans_mask = 0xF81F;
		}
	}

	video = ptr;

	sprintf(desc, "%dx%d %d-bit (VESA LFB)", xres, yres, hicolor ? hicolor : 8);
	DriverDesc = desc;

	true_screen_width = xres;
	true_screen_length = yres;

	ShowPage = LFB_ShowPage;
	ShutdownVideo = Mode13hShutdown;

	return 1; // success
}

int GFX_SetMode(int xres, int yres)
{
	if (ShutdownVideo)
		ShutdownVideo(0);

// try and get a VESA LFB mode first
	if (!GFX_FindSetModeVESA(xres, yres))
	{
		if (!DriverDesc)
		{
			Sys_Error("GFX_SetMode: unable to set hicolor %dx%d", xres, yres);
		}
		if (!hicolor)
		{
		// couldn't get VESA, try for ModeX or Mode$13; 8-bit color only
			if (!GFX_FindSetMode(xres, yres))
				return 0; // failure

			video = (unsigned char *) 0xA0000;
		}
	}

// we were successfull; allocate virtual buffer and setup other screen info
	vscreen = (byte *) valloc(true_screen_width*true_screen_length*(hicolor?2:1), "vscreen", 0);
	screen = vscreen;
	screen_width = true_screen_width;
	screen_length = true_screen_length;
	clip.x = 0;
	clip.y = 0;
	clip.xend = screen_width - 1;
	clip.yend = screen_length - 1;

// make sure palette persists
	GFX_SetPaletteIntensity(64);

	LogDone();

	return 1; // success
}