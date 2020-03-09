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
// ³                    Video Driver Manager module                      ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <aen, may 5>
// + added routines for silhouetted blits:
//   Silhouette, SilhouetteZoom, SilhouetteLucent, SilhouetteZoomLucent
//   + clipping versions
//   all are transparent blits, with non-zero pixels in the given color.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//#define VDRIVER_H

//#include <malloc.h>
#include <mem.h>
#include <conio.h>
#include <math.h>

#include "verge.h"

#define SWAP(a,b) { a-=b; b+=a; a=b-a; }

// ================================= Data ====================================

rect_t clip = { 0, 0, 0, 0, };

int		true_screen_width	= 0;
int		true_screen_length	= 0;
int		screen_width		= 0;
int		screen_length		= 0;

int		vsync				= 0;

char*	DriverDesc			= 0;

unsigned char*	screen		= 0;
unsigned char*	video		= 0;
unsigned char*	vscreen		= 0;

//byte	pal[768]={0};
unsigned char	game_palette[3*256];

unsigned char* translucency_table	= 0;

// Driver function-pointers.
int (*ShutdownVideo) (int text_mode)	= 0;
int (*ShowPage) (void)	= 0;

// ================================= Code ====================================

#define DAC_WRITE 0x03c8
#define DAC_DATA  0x03c9
#define DAC_READ  0x03c7

//accepts a palette and sticks it on the screen
//first we convert 0..63 palette colors into 0..255 and then slap it into directdraw
//todo:we can speed up the conversion loop
void SetPalette(byte *pal)
{
	int i;
	for(i=0; i<256; i++)	//the conversion
	{
		vdrvwin8_CurrPalData[x].peRed=source_palette[i*3]*4;
		vdrvwin8_CurrPalData[x].peGreen=source_palette[i*3+1]*4;
		vdrvwin8_CurrPalData[x].peBlue=source_palette[i*3+2]*4;
		vdrvwin8_CurrPalData[x].peFlags=0;
	}
	IDirectDrawPalette_SetEntries(vdrvwin8_CurrDDPal,0,0,256,vdrvwin8_CurrPalData);	//install the palette
	IDirectDrawSurface_SetPalette(ps,vdrvwin8_CurrDDPal);	//make it active
}


//takes the actual palette on the screen, and puts it in pal[]
//not too useful...
//just opposite of SetPalette().
//todo:same thing
void GetPalette()
{
	int i;
	for(i=0; i<256; i++)
	{
		pal[i*3]=vdrvwin8_CurrPalData[x].peRed/4;
		pal[i*3+1]=vdrvwin8_CurrPalData[x].peGreen/4;
		pal[i*3+2]=vdrvwin8_CurrPalData[x].peBlue/4;
	}
}

void GFX_SetPalette(unsigned char* source_palette)
{
	int		n;

// debugging for now
	if (hicolor) return;

	outp(DAC_WRITE, 0);
	for (n = 0; n < 3*256; n += 1)
	{
		outp(DAC_DATA, source_palette[n]);
	}
}

void GFX_GetPalette()
{
	int		n;

// debugging for now
	if (hicolor) return;

	outp(DAC_READ, 0);
	for (n = 0; n < 3*256; n += 1)
	{
		game_palette[n] = (unsigned char) inp(DAC_DATA);
	}
}

void GFX_SetPaletteIntensity(int intensity)
{
	int		n;
	unsigned char	source_palette[3*256];

	if (intensity<0)
		intensity=0;
	if (intensity>64)
		intensity=64;

	for (n = 0; n < 3*256; n += 1)
	{
		source_palette[n] = (unsigned char) ((game_palette[n]*intensity) >> 6);
	}

	GFX_SetPalette(source_palette);
}

void LFB_Blit_8(
	int x, int y, int width, int length, unsigned char* source,
	int masked, int color_mapped);

int LFB_ShowPage(void)
{
	RenderGUI();

	cpubyte = PFLIP;

	if (true_screen_width == screen_width && true_screen_length == screen_length)
	{
		V_memcpy(video, screen, screen_width*screen_length*(hicolor?2:1));
	}
// be a little more friendly
	else
	{
	// debugging hicolor
		if (hicolor) return 0;

		unsigned char*	save_screen;
		rect_t save_clip;

	// save
		save_screen = screen;
		save_clip = clip;

	// temp set
		screen = video; // DJGPP fix point
		clip.x = 0;
		clip.y = 0;
		clip.xend = true_screen_width - 1;
		clip.yend = true_screen_length - 1;

	// hee hee
		LFB_Blit_8(0, 0, screen_width, screen_length, screen, 0, 0);

	// restore
		screen = save_screen;
		clip = save_clip;
	}

	cpubyte = ETC;

	CheckHookTimer();

	return 0;
}

#pragma off (unreferenced);

/*
static void wmemcpy(unsigned short* dest, unsigned short* source, int len)
{
	__asm
	{
		mov esi,source
		mov edi,dest
		mov ecx,len
		cld
		rep movsw
	};
}
*/

static void LFB_Blit_16(
	int x, int y, int width, int length, unsigned short* source,
	int masked, int color_mapped)
{
	unsigned short*	dest;
	int		clip_width;

	cpubyte = RENDER;

// nothing is visible; easy out!
	if (x > clip.xend || x + width - 1 < clip.x
	||	y > clip.yend || y + length - 1 < clip.y)
	{
		return;
	}

	clip_width=width;
// clip lower right
	if (x + clip_width - 1 > clip.xend)
	{
		clip_width = clip.xend - x + 1;
	}
	if (y + length - 1 > clip.yend)
	{
		length = clip.yend - y + 1;
	}
// clip upper left
	if (x < clip.x)
	{
		source += (clip.x - x);
		clip_width -= (clip.x - x);
		x = clip.x;
	}
	if (y<clip.y)
	{
		source += (clip.y - y)*width;
		length -= (clip.y - y);
		y = clip.y;
	}

	dest = ((unsigned short *) screen) + y*screen_width + x;

	if (color_mapped)
	{
	// color_mapped && masked
		if (masked)
		{
				int		c;
				do
				{
					x = clip_width;
					do
					{
						c = *source;
						if (c != trans_mask)
						{
							c = (c & tmask) + (*dest & tmask);
							*dest = (unsigned short) (c >> 1);
						}
						source += 1;
						dest += 1;
						x -= 1;
					}
					while (x);

					source += (width - clip_width);
					dest += (screen_width - clip_width);
					length -= 1;
				}
				while (length);
		}
	// color_mapped && solid
		else
		{
			int		c;
			do
			{
				x = clip_width;
				do
				{

					c = (*source & tmask) + (*dest & tmask);
					*dest = (unsigned short) (c >> 2);

					source += 1;
					dest += 1;
					x -= 1;
				}
				while (x);

				source += (width - clip_width);
				dest += (screen_width - clip_width);
				length -= 1;
			}
			while (length);
		}
	}
	else
	{
	// masked
		if (masked)
		{
			int		c;
			do
			{
				x = clip_width;
				do
				{
					c = *source;
					if (c != trans_mask)
					{
						*dest = (unsigned short)c;
					}
					source += 1;
					dest += 1;
					x -= 1;
				}
				while (x);

				source += (width - clip_width);
				dest += (screen_width - clip_width);
				length -= 1;
			}
			while (length);
		}
	// solid
		else
		{
			do
			{
				for (x=0; x<clip_width; x++)
					dest[x] = source[x];
				//wmemcpy(dest, source, clip_width);

				source += width; // - clip_width);
				dest += screen_width; // - clip_width);
				length -= 1;
			} while (length);
		}
	}

	cpubyte = ETC;
}

void LFB_Blit_8(
	int x, int y, int width, int length, unsigned char* source,
	int masked, int color_mapped)
{
	unsigned char*	dest;
	int		clip_width;

	cpubyte = RENDER;

// nothing is visible; easy out!
	if (x > clip.xend || x + width - 1 < clip.x
	||	y > clip.yend || y + length - 1 < clip.y)
	{
		return;
	}

	clip_width=width;
// clip lower right
	if (x + clip_width - 1 > clip.xend)
	{
		clip_width = clip.xend - x + 1;
	}
	if (y + length - 1 > clip.yend)
	{
		length = clip.yend - y + 1;
	}
// clip upper left
	if (x < clip.x)
	{
		source += (clip.x - x);
		clip_width -= (clip.x - x);
		x = clip.x;
	}
	if (y<clip.y)
	{
		source += (clip.y - y)*width;
		length -= (clip.y - y);
		y = clip.y;
	}

	dest = screen + y*screen_width + x;

	if (color_mapped && translucency_table)
	{
	// color_mapped && masked
		if (masked)
		{
			int		c;
			do {
				x = clip_width;
				do
				{
					c = *source;
					if (c)
					{
						*dest = translucency_table[*dest | (c << 8)];
					}
					source += 1;
					dest += 1;
					x -= 1;
				} while (x);

				source += (width - clip_width);
				dest += (screen_width - clip_width);
				length -= 1;
			} while (length);
		}
	// color_mapped && solid
		else
		{
			do
			{
				x = clip_width;
				do
				{
					*dest = translucency_table[*dest | (*source << 8)];

					source += 1;
					dest += 1;
					x -= 1;
				} while (x);

				source += (width - clip_width);
				dest += (screen_width - clip_width);
				length -= 1;
			} while (length);
		}
	}
	else
	{
	// masked
		if (masked)
		{
			int		c;
			do {
				x = clip_width;
				do
				{
					c = *source;
					if (c)
					{
						*dest = (unsigned char)c;
					}
					source += 1;
					dest += 1;
					x -= 1;
				} while (x);

				source += (width - clip_width);
				dest += (screen_width - clip_width);
				length -= 1;
			} while (length);
		}
	// solid
		else
		{
			do
			{
				V_memcpy(dest, source, clip_width);

				source += width;
				dest += screen_width;
				length -= 1;
			} while (length);
		}
	}

	cpubyte = ETC;
}

void LFB_Blit(int x, int y, int width, int length, unsigned char* source,
	int masked, int color_mapped)
{
	if (hicolor)
		LFB_Blit_16(x, y, width, length, (unsigned short *) source, masked, color_mapped);
	else
		LFB_Blit_8(x, y, width, length, source, masked, color_mapped);
}

static void LFB_BlitZoom_8(int x, int y, int sw, int sh, int dw, int dh, unsigned char* src,
	int masked, int color_mapped)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	unsigned char* d;
	int xl, yl, xs, ys;

	cpubyte = RENDER;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;
	if (x > clip.xend || y > clip.yend
	|| x + xl < clip.x || y + yl < clip.y)
		return;
	if (x + xl > clip.xend)
		xl = clip.xend - x + 1;
	if (y + yl > clip.yend)
		yl = clip.yend - y + 1;
	if (x < clip.x) { xs = clip.x - x; xl -= xs; x = clip.x; }
	if (y < clip.y) { ys = clip.y - y; yl -= ys; y = clip.y; }

	xadj = (sw << 16)/dw;
	yadj = (sh << 16)/dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	src += ((yerr_start >> 16)*sw);
	d = screen + (y*screen_width) + x;
	yerr = yerr_start & 0xffff;

	if (color_mapped && translucency_table)
	{
		if (masked)
		{
			int		c;
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					c = src[(xerr >> 16)];
					if (c)
						d[j] = translucency_table[d[j] | (c << 8)];
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr >> 16)*sw;
				yerr &= 0xffff;
			}
		}
		else
		{
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					d[j] = translucency_table[d[j] | (src[(xerr >> 16)] << 8)];
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr >> 16)*sw;
				yerr &= 0xffff;
			}
		}
	}
	else
	{
		if (masked)
		{
			int		c;
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					c = src[(xerr >> 16)];
					if (c)
						d[j] = (unsigned char) c;
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr >> 16)*sw;
				yerr &= 0xffff;
			}
		}
		else
		{
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					d[j] = src[(xerr >> 16)];
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr>>16)*sw;
				yerr &= 0xffff;
			}
		}
	}

	cpubyte = ETC;
}

static void LFB_BlitZoom_16(int x, int y, int sw, int sh, int dw, int dh, unsigned short* src,
	int masked, int color_mapped)
{
	int i, j;
	int xerr, yerr;
	int xerr_start, yerr_start;
	int xadj, yadj;
	unsigned short* d;
	int xl, yl, xs, ys;

	cpubyte = RENDER;

	if (dw < 1 || dh < 1)
		return;

	xl = dw;
	yl = dh;
	xs = ys = 0;
	if (x > clip.xend || y > clip.yend
	|| x + xl < clip.x || y + yl < clip.y)
		return;
	if (x + xl > clip.xend)
		xl = clip.xend - x + 1;
	if (y + yl > clip.yend)
		yl = clip.yend - y + 1;
	if (x < clip.x) { xs = clip.x - x; xl -= xs; x = clip.x; }
	if (y < clip.y) { ys = clip.y - y; yl -= ys; y = clip.y; }

	xadj = (sw << 16)/dw;
	yadj = (sh << 16)/dh;
	xerr_start = xadj * xs;
	yerr_start = yadj * ys;

	src += ((yerr_start >> 16)*sw);
	d = ((unsigned short *) screen) + (y*screen_width) + x;
	yerr = yerr_start & 0xffff;

	if (color_mapped)
	{
		if (masked)
		{
			int		c;
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					c = src[(xerr >> 16)];
					if (c != trans_mask)
					{
						c = (c & tmask) + (d[j] & tmask);
						d[j] = (unsigned short) (c >> 1);
					}
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr >> 16)*sw;
				yerr &= 0xffff;
			}
		}
		else
		{
			int		c;
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					c = (src[(xerr >> 16)] & tmask) + (d[j] & tmask);
					d[j] = (unsigned short) (c >> 1);

					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr >> 16)*sw;
				yerr &= 0xffff;
			}
		}
	}
	else
	{
		if (masked)
		{
			int		c;
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					c = src[(xerr >> 16)];
					if (c != trans_mask)
						d[j] = (unsigned short) c;
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr >> 16)*sw;
				yerr &= 0xffff;
			}
		}
		else
		{
			for (i = 0; i < yl; i += 1)
			{
				xerr = xerr_start;
				for (j = 0; j < xl; j += 1)
				{
					d[j] = src[(xerr >> 16)];
					xerr += xadj;
				}
				d    += screen_width;
				yerr += yadj;
				src  += (yerr>>16)*sw;
				yerr &= 0xffff;
			}
		}
	}

	cpubyte = ETC;
}

void LFB_BlitZoom(int x, int y, int sw, int sh, int dw, int dh, unsigned char* src,
	int masked, int color_mapped)
{
	if (hicolor)
		LFB_BlitZoom_16(x, y, sw, sh, dw, dh, (unsigned short *) src, masked, color_mapped);
	else
		LFB_BlitZoom_8(x, y, sw, sh, dw, dh, src, masked, color_mapped);
}

static void LFB_BlitBop_8(int x, int y, int width, int length, int color, unsigned char* source,
	int color_mapped)
{
	unsigned char*	dest;
	int		clip_width;

	cpubyte = RENDER;

// nothing is visible; easy out!
	if (x > clip.xend || x + width - 1 < clip.x
	||	y > clip.yend || y + length - 1 < clip.y)
	{
		return;
	}

	clip_width=width;
// clip lower right
	if (x + clip_width - 1 > clip.xend)
	{
		clip_width = clip.xend - x + 1;
	}
	if (y + length - 1 > clip.yend)
	{
		length = clip.yend - y + 1;
	}
// clip upper left
	if (x < clip.x)
	{
		source += (clip.x - x);
		clip_width -= (clip.x - x);
		x = clip.x;
	}
	if (y<clip.y)
	{
		source += (clip.y - y)*width;
		length -= (clip.y - y);
		y = clip.y;
	}

	dest = screen + y*screen_width + x;
	color &= 0xff;

// color mapped
	if (color_mapped && translucency_table)
	{
		do {
			x = clip_width;
			do
			{
				if (*source)
				{
					*dest = translucency_table[*dest | (color << 8)];
				}
				source += 1;
				dest += 1;
				x -= 1;
			} while (x);

			source += (width - clip_width);
			dest += (screen_width - clip_width);
			length -= 1;
		} while (length);
	}
// solid
	else
	{
		do {
			x = clip_width;
			do
			{
				if (*source)
				{
					*dest = (unsigned char)color;
				}
				source += 1;
				dest += 1;
				x -= 1;
			} while (x);

			source += (width - clip_width);
			dest += (screen_width - clip_width);
			length -= 1;
		} while (length);
	}

	cpubyte = ETC;
}

void LFB_BlitBop(int x, int y, int width, int length, int color, unsigned char* source,
	int color_mapped)
{
	if (hicolor)
		return;
	else
		LFB_BlitBop_8(x, y, width, length, color, source, color_mapped);
}

/*
// unused
void LFB_BlitCoat_8(int x, int y, int width, int length, int color, unsigned char* source)
{
	unsigned char*	dest;
	unsigned char*	range;
	int		clip_width, c;

	cpubyte = RENDER;

// nothing is visible; easy out!
	if (x > clip.xend || x + width - 1 < clip.x
	||	y > clip.yend || y + length - 1 < clip.y)
	{
		return;
	}

	clip_width=width;
// clip lower right
	if (x + clip_width - 1 > clip.xend)
	{
		clip_width = clip.xend - x + 1;
	}
	if (y + length - 1 > clip.yend)
	{
		length = clip.yend - y + 1;
	}
// clip upper left
	if (x < clip.x)
	{
		source += (clip.x - x);
		clip_width -= (clip.x - x);
		x = clip.x;
	}
	if (y<clip.y)
	{
		source += (clip.y - y)*width;
		length -= (clip.y - y);
		y = clip.y;
	}

	dest = screen + y*screen_width + x;
	color &= 0xff;
	range = translucency_table + (color << 8);

	do {
		x = clip_width;
		do
		{
			c = *source;
			if (c)
			{
				*dest = range[c];
			}
			source += 1;
			dest += 1;
			x -= 1;
		}
		while (x);
		source += (width - clip_width);
		dest += (screen_width - clip_width);
		length -= 1;
		}
		while (length);
	}

	cpubyte = ETC;
}
*/

static void LFB_BlitTileRow(
	int x, int y, int y_offset, int tile, int masked, int color_mapped)
{
	if (tile < 0 || tile >= numtiles)
		return;
	if (y_offset < 0 || y_offset >= 16)
		return;

	if (hicolor)
		LFB_Blit_16(x, y, 16, 1, ((unsigned short *)vsp) + 16*16*tileidx[tile] + (16*y_offset), masked, color_mapped);
	else
		LFB_Blit_8(x, y, 16, 1, vsp + 16*16*tileidx[tile] + (16*y_offset), masked, color_mapped);
}

// be a little careful here; no way to validate source currently.
void LFB_BlitMapLine(
	int x, int y, int y_offset, unsigned short* source, int masked, int color_mapped)
{
	if (masked)
	{
		do
		{
			if (*source)
				LFB_BlitTileRow(x, y, y_offset, *source, masked, color_mapped);

			source += 1;
			x += 16;
		}
		while (x < true_screen_width);
	}
	else
	{
		do
		{
			LFB_BlitTileRow(16, y, y_offset, *source, masked, color_mapped);

			source += 1;
			x += 16;
		}
		while (x < true_screen_width);
	}
}

void LFB_ClearScreen()
{
	cpubyte = RENDER;

	V_memset(screen, 0, screen_width*screen_length*(hicolor?2:1));

	cpubyte = ETC;
}

static void LFB_BlitPixel_8(int x, int y, int color, int color_mapped)
{
	if (x < clip.x || y < clip.y || x > clip.xend || y > clip.yend)
		return;

	if (color_mapped && translucency_table)
	{
		unsigned char*	dest;
		dest = screen + (y*screen_width) + x;
		*dest = translucency_table[*dest | (color << 8)];
	}
	else
		screen[(y*screen_width) + x] = (unsigned char) color;
}

static void LFB_BlitPixel_16(int x, int y, int color, int color_mapped)
{
	if (x < clip.x || y < clip.y || x > clip.xend || y > clip.yend)
		return;

	if (color_mapped)
	{
		unsigned short*	dest;
		int		c;
		dest = ((unsigned short *) screen) + (y*screen_width) + x;
		c = (color & tmask) + (*dest & tmask);
		*dest = (unsigned short) (c >> 1);
	}
	else
		((unsigned short *) screen)[(y*screen_width) + x] = (unsigned short) color;
}

void LFB_BlitPixel(int x, int y, int color, int color_mapped)
{
	if (hicolor)
		LFB_BlitPixel_16(x, y, color, color_mapped);
	else
		LFB_BlitPixel_8(x, y, color, color_mapped);
}

static int LFB_GetPixel_8(int x, int y)
{
	if (x < clip.x || y < clip.y || x > clip.xend || y > clip.yend)
		return 0;

	return screen[(y*screen_width) + x];
}

static int LFB_GetPixel_16(int x, int y)
{
	if (x < clip.x || y < clip.y || x > clip.xend || y > clip.yend)
		return 0;

	return ((unsigned short *) screen)[(y*screen_width) + x];
}

int LFB_GetPixel(int x, int y)
{
	if (hicolor)
		return LFB_GetPixel_16(x, y);
	else
		return LFB_GetPixel_8(x, y);
}

void LFB_BlitStippleTile(int x, int y, int color)
{
	int		n;
	void (*BlitPixel)(int, int, int, int);

	cpubyte = RENDER;

	BlitPixel = hicolor ? LFB_BlitPixel_16 : LFB_BlitPixel_8;

	n = 0;
	do
	{
		BlitPixel(x + 0,  y + n, color, 0);
		BlitPixel(x + 2,  y + n, color, 0);
		BlitPixel(x + 4,  y + n, color, 0);
		BlitPixel(x + 6,  y + n, color, 0);
		BlitPixel(x + 8,  y + n, color, 0);
		BlitPixel(x + 10, y + n, color, 0);
		BlitPixel(x + 12, y + n, color, 0);
		BlitPixel(x + 14, y + n, color, 0);
		n++;

		BlitPixel(x + 0,  y + n, color, 0);
		BlitPixel(x + 2,  y + n, color, 0);
		BlitPixel(x + 4,  y + n, color, 0);
		BlitPixel(x + 6,  y + n, color, 0);
		BlitPixel(x + 8,  y + n, color, 0);
		BlitPixel(x + 10, y + n, color, 0);
		BlitPixel(x + 12, y + n, color, 0);
		BlitPixel(x + 14, y + n, color, 0);
		n++;
	}
	while (n < 16);

	cpubyte = ETC;
}

static void LFB_BlitHLine_8(int x, int y, int x2, int color, int color_mapped)
{
	int width;

	cpubyte = RENDER;

	if (x2 < x) SWAP(x, x2);
	width = x2 - x + 1;
	if (x > clip.xend || y > clip.yend || x + width <= clip.x || y < clip.y)
		return;
	if (x + width - 1 > clip.xend) width = clip.xend - x + 1;
	if (x < clip.x) { width -= (clip.x - x); x = clip.x; }

	if (color_mapped && translucency_table)
	{
		int		c;
		unsigned char*	dest;
		c = color << 8;
		dest = screen + (y*screen_width) + x;

		x = 0;
		do
		{
			*dest = translucency_table[c | *dest];

			x += 1;
			dest += 1;
		}
		while (x < width);
	}
	else
	{
		V_memset(screen + (y*screen_width) + x, color, width);
	}

	cpubyte = ETC;
}

static void LFB_BlitHLine_16(int x, int y, int x2, int color, int color_mapped)
{
	int width;

	cpubyte = RENDER;

	if (x2 < x) SWAP(x, x2);
	width = x2 - x + 1;
	if (x > clip.xend || y > clip.yend || x + width <= clip.x || y < clip.y)
		return;
	if (x + width - 1 > clip.xend) width = clip.xend - x + 1;
	if (x < clip.x) { width -= (clip.x - x); x = clip.x; }

	if (color_mapped)
	{
		int		c;
		unsigned short*	dest;
		dest = ((unsigned short *) screen) + (y*screen_width) + x;

		x = 0;
		do
		{
			c = (color & tmask) + (*dest & tmask);
			*dest = (unsigned short) (c >> 1);

			x += 1;
			dest += 1;
		}
		while (x < width);
	}
	else
	{
		unsigned short* dest;
		dest = ((unsigned short *) screen) + (y*screen_width) + x;
		for (x = 0; x < width; x += 1, dest += 1)
			*dest = (unsigned short) color;
		//V_memset(screen + (y*screen_width) + x, color, width);
	}

	cpubyte = ETC;
}

void LFB_BlitHLine(int x, int y, int x2, int color, int color_mapped)
{
	if (hicolor)
		LFB_BlitHLine_16(x, y, x2, color, color_mapped);
	else
		LFB_BlitHLine_8(x, y, x2, color, color_mapped);
}

static void LFB_BlitVLine_8(int x, int y, int y2, int color, int color_mapped)
{
	unsigned char*	dest;
	int		length;

	cpubyte = RENDER;

	if (y2 < y) SWAP(y, y2);
	length = y2 - y + 1;
	if (x > clip.xend || y > clip.yend || x < clip.x || y + length <= clip.y)
	{
		cpubyte = ETC;
		return;
	}
	if (y + length - 1 > clip.yend) length = clip.yend - y + 1;
	if (y < clip.y) { length -= (clip.y - y); y = clip.y; }

	if (color_mapped && translucency_table)
	{
		int		c;
		c = color << 8;
		dest = screen + (y*screen_width) + x;
		do
		{
			*dest = translucency_table[c | *dest];

			dest += screen_width;
			length -= 1;
		}
		while (length);
	}
	else
	{
		dest = screen + (y*screen_width) + x;
		do
		{
			*dest = (unsigned char) color;

			dest += screen_width;
			length -= 1;
		}
		while (length);
	}

	cpubyte = ETC;
}

static void LFB_BlitVLine_16(int x, int y, int y2, int color, int color_mapped)
{
	unsigned short*	dest;
	int		length;

	cpubyte = RENDER;

	if (y2 < y) SWAP(y, y2);
	length = y2 - y + 1;
	if (x > clip.xend || y > clip.yend || x < clip.x || y + length <= clip.y)
	{
		cpubyte = ETC;
		return;
	}
	if (y + length - 1 > clip.yend) length = clip.yend - y + 1;
	if (y < clip.y) { length -= (clip.y - y); y = clip.y; }

	if (color_mapped)
	{
		int		c;
		//c = color << 8;
		dest = ((unsigned short *) screen) + (y*screen_width) + x;
		do
		{
			c = (color & tmask) + (*dest & tmask);
			*dest = (unsigned short) (c >> 1);
			//*dest = translucency_table[c | *dest];

			dest += screen_width;
			length -= 1;
		}
		while (length);
	}
	else
	{
		dest = ((unsigned short *) screen) + (y*screen_width) + x;
		do
		{
			*dest = (unsigned short) color;

			dest += screen_width;
			length -= 1;
		}
		while (length);
	}

	cpubyte = ETC;
}

void LFB_BlitVLine(int x, int y, int y2, int color, int color_mapped)
{
	if (hicolor)
		LFB_BlitVLine_16(x, y, y2, color, color_mapped);
	else
		LFB_BlitVLine_8(x, y, y2, color, color_mapped);
}

static void LFB_BlitLine_8(int x1, int y1, int x2, int y2, int color, int color_mapped)
{
	int i, xc, yc, er, n, m, xi, yi, xcxi, ycyi, xcyi;
	unsigned int dcy, dcx;

	cpubyte = RENDER;

// check to see if the line is completly clipped off
	if ((x1 < clip.x && x2 < clip.x) || (x1 > clip.xend && x2 > clip.xend)
	||	(y1 < clip.y && y2 < clip.y) || (y1 > clip.yend && y2 > clip.yend))
	{
		cpubyte = ETC;
		return;
	}

	if (x1 > x2)
	{
		SWAP(x1, x2);
		SWAP(y1, y2);
	}

// clip the left side
	if (x1 < clip.x)
	{
		int myy = (y2 - y1);
		int mxx = (x2 - x1), b;
		if (!mxx)
		{
			cpubyte = ETC;
			return;
		}
		if (myy)
		{
			b = y1 - (y2 - y1)*x1/mxx;
			y1 = myy*clip.x/mxx + b;
			x1 = clip.x;
		}
		else
			x1 = clip.x;
	}

// clip the right side
	if (x2 > clip.xend)
	{
		int myy = (y2 - y1);
		int mxx = (x2 - x1), b;
		if (!mxx)
		{
			cpubyte = ETC;
			return;
		}
		if (myy)
		{
			b = y1 - (y2 - y1)*x1/mxx;
			y2 = myy*clip.xend/mxx + b;
			x2 = clip.xend;
		}
		else
			x2 = clip.xend;
	}

	if (y1 > y2)
	{
		SWAP(x1, x2);
		SWAP(y1, y2);
	}

// clip the bottom
	if (y2 > clip.yend)
	{
		int mxx = (x2 - x1);
		int myy = (y2 - y1), b;
		if (!myy)
		{
			cpubyte = ETC;
			return;
		}
		if (mxx)
		{
			b = y1 - (y2 - y1)*x1/mxx;
			x2 = (clip.yend - b)*mxx/myy;
			y2 = clip.yend;
		}
		else
			y2 = clip.yend;
	}

// clip the top
	if (y1 < clip.y)
	{
		int mxx = (x2 - x1);
		int myy = (y2 - y1), b;
		if (!myy)
		{
			cpubyte = ETC;
			return;
		}
		if (mxx)
		{
			b = y1 - (y2 - y1)*x1/mxx;
			x1 = (clip.y - b)*mxx/myy;
			y1 = clip.y;
		}
		else
			y1 = clip.y;
	}

// see if it got cliped into the box, out out
	if (x1 < clip.x || x2 < clip.x || x1 > clip.xend || x2 > clip.xend
	||	y1 < clip.y || y2 < clip.y || y1 > clip.yend || y2 > clip.yend)
	{
		cpubyte = ETC;
		return;
	}

	if (x1 > x2)
	{
		xc = x2;
		xi = x1;
	}
	else
	{
		xi = x2;
		xc = x1;
	}

// assume y1<=y2 from above swap operation
	yi = y2;
	yc = y1;

	dcx = x1;
	dcy = y1;
	xc = (x2 - x1);
	yc = (y2 - y1);
	if (xc < 0)
		xi = -1;
	else
		xi = 1;
	if (yc < 0)
		yi = -1;
	else
		yi=1;
	n = abs(xc);
	m = abs(yc);
	ycyi = abs(2*yc*xi);
	er = 0;

	if (color_mapped && translucency_table)
	{
		unsigned char*	dest;
		int		c;
		c = (color & 0xff) << 8;
		if (n>m)
		{
			xcxi = abs(2*xc*xi);
			for (i = 0; i <= n; i += 1)
			{
				dest = screen + (dcy*screen_width) + dcx;
				*dest = translucency_table[c | *dest];
				if (er > 0)
				{
					dcy += yi;
					er -= xcxi;
				}
				er += ycyi;
				dcx += xi;
			}
		}
		else
		{
			xcyi = abs(2*xc*yi);
			for (i = 0; i <= m; i += 1)
			{
				dest = screen + (dcy*screen_width) + dcx;
				*dest = translucency_table[c | *dest];
				if (er > 0)
				{
					dcx += xi;
					er -= ycyi;
				}
				er += xcyi;
				dcy += yi;
			}
		}
	}
	else
	{
		if (n>m)
		{
			xcxi = abs(2*xc*xi);
			for (i = 0; i <= n; i += 1)
			{
				screen[(dcy*screen_width) + dcx] = (unsigned char) color;
				if (er > 0)
				{
					dcy += yi;
					er -= xcxi;
				}
				er += ycyi;
				dcx += xi;
			}
		}
		else
		{
			xcyi = abs(2*xc*yi);
			for (i = 0; i <= m; i += 1)
			{
				screen[(dcy*screen_width) + dcx] = (unsigned char) color;
				if (er > 0)
				{
					dcx += xi;
					er -= ycyi;
				}
				er += xcyi;
				dcy += yi;
			}
		}
	}

	cpubyte = ETC;
}

void LFB_BlitLine(int x1, int y1, int x2, int y2, int color, int color_mapped)
{
	if (hicolor)
		return;
	else
		LFB_BlitLine_8(x1, y1, x2, y2, color, color_mapped);
}

void LFB_BlitCircle(int x, int y, int radius, int color, int filled, int color_mapped)
{
	int cx = 0;
	int cy = radius;
	int df = 1 - radius;
	int d_e = 3;
	int d_se = -2*radius + 5;

	cpubyte = RENDER;
	if (filled)
	{
		void (*BlitHLine)(int, int, int, int, int);
		BlitHLine = hicolor ? LFB_BlitHLine_16 : LFB_BlitHLine_8;

		do
		{
			BlitHLine(x - cy, y - cx, x + cy, color, color_mapped);
			if (cx) BlitHLine(x - cy, y + cx, x + cy, color, color_mapped);

			if (df < 0)
			{
				df += d_e;
				d_e += 2;
				d_se += 2;
			}
			else
			{
				if (cx != cy)
				{
					BlitHLine(x - cx, y - cy, x + cx, color, color_mapped);
					if (cy) BlitHLine(x - cx, y + cy, x + cx, color, color_mapped);
				}
				df += d_se;
				d_e += 2;
				d_se += 4;
				cy -= 1;
			}
			cx += 1;
		}
		while (cx <= cy);
	}
	else
	{
		void (*BlitPixel)(int, int, int, int);
		BlitPixel = hicolor ? LFB_BlitPixel_16 : LFB_BlitPixel_8;

		do
		{
			BlitPixel(x + cx, y + cy, color, color_mapped);
			if (cx) BlitPixel(x - cx, y + cy, color, color_mapped);
			if (cy) BlitPixel(x + cx, y - cy, color, color_mapped);
			if (cx && cy) BlitPixel(x - cx, y - cy, color, color_mapped);

			if (cx != cy)
			{
				BlitPixel(x + cy, y + cx, color, color_mapped);
				if (cx) BlitPixel(x + cy, y - cx, color, color_mapped);
				if (cy) BlitPixel(x - cy, y + cx, color, color_mapped);
				if (cx && cy) BlitPixel(x - cy, y - cx, color, color_mapped);
			}

			if (df < 0)
			{
				df += d_e;
				d_e += 2;
				d_se += 2;
			}
			else
			{
				df += d_se;
				d_e += 2;
				d_se += 4;
				cy -= 1;
			}
			cx += 1;
		}
		while (cx <= cy);
	}

	cpubyte = ETC;
}

void LFB_BlitRect(int x, int y, int x2, int y2, int color, int filled, int color_mapped)
{
	void (*HLine)(int, int, int, int, int);

	HLine = hicolor ? LFB_BlitHLine_16 : LFB_BlitHLine_8;

	if (filled)
	{
		cpubyte = RENDER;
		if (y2 < y) SWAP(y, y2);
		do
		{
			HLine(x, y, x2, color, color_mapped);
			y += 1;
		}
		while (y <= y2);
		cpubyte = ETC;
	}
	else
	{
		void (*VLine)(int, int, int, int, int);
		VLine = hicolor ? LFB_BlitVLine_16 : LFB_BlitVLine_8;

		HLine(x,  y,     x2,     color, color_mapped);
		HLine(x,  y2,    x2,     color, color_mapped);
		VLine(x,  y + 1, y2 - 1, color, color_mapped);
		VLine(x2, y + 1, y2 - 1, color, color_mapped);
	}
}

// ============================================================================
// =                            Wrap Blitters                                 =
// ============================================================================

void LFB_BlitWrap(
	quad x, quad y, int width, int length, unsigned char* source,
	int masked, int color_mapped)
{
	int cur_x, sign_y;

	if (width < 1 || length < 1)
		return;

	x %= width, y %= length;
	sign_y = 0 - y;

	for (; sign_y < screen_length; sign_y += length)
	{
		for (cur_x = 0 - x; cur_x < screen_width; cur_x += width)
			LFB_Blit(cur_x, sign_y, width, length, source, masked, color_mapped);
	}
}

// ============================================================================
// =           alias.zip / zero / aen rotational scaler routines              =
// ============================================================================

static void LFB_BlitRotZoom_8(int posx, int posy, quad width, quad height, float angle, float scale,
	unsigned char* src, int masked, int color_mapped)
{
  // new! shamelessly ripped off from alias.zip
  // except the atan2 stuff which i had to make up myself AEN so there :p

  int xs,ys,xl,yl;
  int sinas,cosas,xc,yc,srcx,srcy,x,y,tempx,tempy,T_WIDTH_CENTER,T_HEIGHT_CENTER,W_WIDTH_CENTER,W_HEIGHT_CENTER,W_HEIGHT,W_WIDTH;
  unsigned char *dest;
  unsigned char pt;
  float ft;

  ft=atan2((float)width,(float)height);

  T_WIDTH_CENTER=width>>1;
  T_HEIGHT_CENTER=height>>1;
  W_WIDTH=((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
  W_HEIGHT=W_WIDTH;
  W_HEIGHT_CENTER=W_HEIGHT>>1;
  W_WIDTH_CENTER=W_HEIGHT_CENTER; //W_WIDTH/2;

  sinas=sin(-angle)*65536*scale;
  cosas=cos(-angle)*65536*scale;

  xc=T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
  yc=T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
  posx-=W_WIDTH_CENTER;
  posy-=W_HEIGHT_CENTER;

  // clipping
  if (W_WIDTH<2 || W_HEIGHT<2) return;
  xl=W_WIDTH;
  yl=W_HEIGHT;
  xs=ys=0;
  if (posx>clip.xend || posy>clip.yend || posx+xl<clip.x || posy+yl<clip.y)
    return;
  if (posx+xl > clip.xend) xl=clip.xend-posx+1;
  if (posy+yl > clip.yend) yl=clip.yend-posy+1;
  if (posx<clip.x)
  {
    xs=clip.x-posx;
    xl-=xs;
    posx=clip.x;

    xc+=cosas*xs; // woo!
    yc-=sinas*xs;
  }
  if (posy<clip.y)
  {
    ys=clip.y-posy;
    yl-=ys;
    posy=clip.y;

    xc+=sinas*ys; // woo!
    yc+=cosas*ys;
  }

	masked=masked;
	color_mapped=color_mapped;

	dest=screen+posx+posy*screen_width;
	if (masked)
	{
		if (color_mapped && translucency_table)
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						pt=src[tempx+tempy*width];
						if (pt)
							dest[x]=translucency_table[pt|dest[x]];
					}

					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
		else
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						pt=src[tempx+tempy*width];
						if (pt)
							dest[x]=pt;
					}
					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
	}
	else
	{
		if (color_mapped && translucency_table)
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						dest[x]=translucency_table[src[tempx+tempy*width] | dest[x]];
					}

					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
		else
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						dest[x]=src[tempx+tempy*width];
					}

					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
	}
}

static void LFB_BlitRotZoom_16(int posx, int posy, quad width, quad height, float angle, float scale,
	unsigned short* src, int masked, int color_mapped)
{
  // new! shamelessly ripped off from alias.zip
  // except the atan2 stuff which i had to make up myself AEN so there :p

  int xs,ys,xl,yl;
  int sinas,cosas,xc,yc,srcx,srcy,x,y,tempx,tempy,T_WIDTH_CENTER,T_HEIGHT_CENTER,W_WIDTH_CENTER,W_HEIGHT_CENTER,W_HEIGHT,W_WIDTH;
  unsigned short *dest;
  unsigned int pt;
  float ft;

  ft=atan2((float)width,(float)height);

  T_WIDTH_CENTER=width>>1;
  T_HEIGHT_CENTER=height>>1;
  W_WIDTH=((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
  W_HEIGHT=W_WIDTH;
  W_HEIGHT_CENTER=W_HEIGHT>>1;
  W_WIDTH_CENTER=W_HEIGHT_CENTER; //W_WIDTH/2;

  sinas=sin(-angle)*65536*scale;
  cosas=cos(-angle)*65536*scale;

  xc=T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
  yc=T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
  posx-=W_WIDTH_CENTER;
  posy-=W_HEIGHT_CENTER;

  // clipping
  if (W_WIDTH<2 || W_HEIGHT<2) return;
  xl=W_WIDTH;
  yl=W_HEIGHT;
  xs=ys=0;
  if (posx>clip.xend || posy>clip.yend || posx+xl<clip.x || posy+yl<clip.y)
    return;
  if (posx+xl > clip.xend) xl=clip.xend-posx+1;
  if (posy+yl > clip.yend) yl=clip.yend-posy+1;
  if (posx<clip.x)
  {
    xs=clip.x-posx;
    xl-=xs;
    posx=clip.x;

    xc+=cosas*xs; // woo!
    yc-=sinas*xs;
  }
  if (posy<clip.y)
  {
    ys=clip.y-posy;
    yl-=ys;
    posy=clip.y;

    xc+=sinas*ys; // woo!
    yc+=cosas*ys;
  }

	masked=masked;
	color_mapped=color_mapped;

	dest=((unsigned short *)screen)+posx+posy*screen_width;
	if (masked)
	{
		if (color_mapped)
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						pt = src[tempx + tempy*width];
						if (pt != trans_mask)
						{
							pt = (pt & tmask) + (dest[x] & tmask);
							dest[x] = (unsigned short) (pt >> 1);
						}
					}

					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
		else
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						pt = src[tempx + tempy*width];
						if (pt != trans_mask)
						{
							dest[x] = pt;
						}
					}
					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
	}
	else
	{
		if (color_mapped)
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						pt = src[tempx + tempy*width];
						pt = (pt & tmask) + (dest[x] & tmask);
						dest[x] = (unsigned short) (pt >> 1);
					}

					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
		else
		{
			for (y=0; y<yl; y++)
			{
				srcx=xc;
				srcy=yc;

				for (x=0; x<xl; x++)
				{
					tempx=(srcx>>16);
					tempy=(srcy>>16);

					if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
					{
						dest[x]=src[tempx+tempy*width];
					}

					srcx+=cosas;
					srcy-=sinas;
				}

				dest+=screen_width;

				xc+=sinas;
				yc+=cosas;
			}
		}
	}
}

void LFB_BlitRotZoom(int posx, int posy, quad width, quad height, float angle, float scale,
	unsigned char* src, int masked, int color_mapped)
{
	if (hicolor)
		LFB_BlitRotZoom_16(posx, posy, width, height, angle, scale, (unsigned short *) src, masked, color_mapped);
	else
		LFB_BlitRotZoom_8(posx, posy, width, height, angle, scale, src, masked, color_mapped);
}

// ============================================================================
// =                        zero's Mosiac effects                             =
// ============================================================================

//mosaic routines
//TODO-need to asm optimize these badly! they are rather slow!

byte FindPalMatchForMosaic(quad r, quad g, quad b)
{
  quad index;
  quad diffrecord,diff,record;
  diffrecord=100000;
  record=100000;

  for(index=0; index<256; index++)
  {
    diff=abs(r-game_palette[index*3])+abs(g-game_palette[index*3+1])+abs(b-game_palette[index*3+2])+1;
    if(diff<diffrecord)
    {
      diffrecord=diff;
      if(diff==1) return (byte)index;
      record=index;
    }
  }
  return (byte)record;
}

byte *InitMosaicTable(void)
{
  quad r,g,b;
  byte *tmp;
  tmp=(byte *)valloc(64*64*64, "InitMosaicTable:tmp",0); //new byte [64*64*64];
  if (!tmp)
  {
	Sys_Error("InitMosaicTable: memory exhausted on tmp");
  }

  for(r=0; r<64; r++)
    for(g=0; g<64; g++)
      for(b=0; b<64; b++)
        tmp[r*4096+g*64+b]=FindPalMatchForMosaic(r,g,b);
  return tmp;
}

void Mosaic(quad xlevel, quad ylevel, byte *tbl, quad xmin, quad ymin, quad xmax, quad ymax)
{
  quad x,y,rtot,btot,gtot,xloop,yloop,xfier,txlevel;
  byte *src;
  byte pixel;

  for(y=ymin; y<ymax; y+=ylevel)
  {
    for(x=xmin; x<xmax; x+=xlevel)
    {
      rtot=0; gtot=0; btot=0;
      src=y*screen_width+x+screen;
      if(ymax-y<ylevel) ylevel=ymax-y;
      if(xmax-x<xlevel) txlevel=xmax-x; else txlevel=xlevel;
      xfier=ylevel*txlevel;
      for(yloop=0; yloop<ylevel; yloop++)
      {
        for(xloop=0; xloop<txlevel; xloop++)
        {
          rtot+=game_palette[*src*3];
          gtot+=game_palette[*src*3+1];
          btot+=game_palette[*src*3+2];
          src++;
        }
        src+=screen_width-txlevel;
      }
      rtot/=xfier;
      gtot/=xfier;
      btot/=xfier;
      pixel=tbl[rtot*4096+gtot*64+btot];
      src=y*screen_width+x+screen;
      for(yloop=0; yloop<ylevel; yloop++)
      {
        for(xloop=0; xloop<txlevel; xloop++)
        {
          *src=pixel;
          src++;
        }
        src+=screen_width-txlevel;
      }
    }
  }
}
