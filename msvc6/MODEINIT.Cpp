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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include "verge.h"

PALETTEENTRY dd_CurrPalData[256];	//current palette data
LPDIRECTDRAWPALETTE dd_CurrDDPal;	//current directdraw palette


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



int GFX_SetMode(int xres, int yres)
{

	HRESULT hr;
	DDPIXELFORMAT ddpf;

	if (ShutdownVideo)
		ShutdownVideo(0);

	
	ShowCursor(0);
	hr = DirectDrawCreate(NULL,&dd,NULL);
	hr = dd -> SetCooperativeLevel(hMainWnd,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	dd -> SetDisplayMode(xres, yres, 8);
	if (!dd) return 0;

	vmemset(&psd, 0, sizeof (DDSURFACEDESC));
	vmemset(&bsd, 0, sizeof (DDSURFACEDESC));
	psd.dwSize = sizeof (DDSURFACEDESC);
	psd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	psd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_SYSTEMMEMORY;
	psd.dwBackBufferCount = 1;
	dd -> CreateSurface(&psd, &ps, 0);
	bsd.dwSize = sizeof (DDSURFACEDESC);
	bsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
	ps -> GetAttachedSurface(&bsd.ddsCaps, &bs);
	bs -> Lock(0, &bsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, 0);
	screen = (byte *) bsd.lpSurface;

// we were successfull; allocate virtual buffer and setup other screen info
	screen = vscreen;
	screen_width = xres;
	screen_length = yres;
	clip.x = 0;
	clip.y = 0;
	clip.xend = screen_width - 1;
	clip.yend = screen_length - 1;

// make sure palette persists
	GFX_SetPaletteIntensity(64);
	
	return 1; // success
}

void GFX_shutdown(void)
{
	bs -> Release();
	ps -> Release();
	dd -> Release();
	DestroyWindow(hMainWnd);
	hMainWnd=CreateWindowEx((int) NULL,"VDE main window type","VDE",WS_POPUP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),NULL,NULL,hMainInst,NULL);
}