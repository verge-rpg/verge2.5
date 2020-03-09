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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <aen, may 5>
// + added extern decs for silhouette routines
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#if !defined(VDRIVER_H)
#define VDRIVER_H

/*
struct video_mode_t
{
	char	id[256];
	int		width, length;
	int		bpp;
};
extern linked_list modelist;
*/

struct rect_t
{
	int		x, y, xend, yend;
};
extern rect_t clip;
extern int	true_screen_width;
extern int 	true_screen_length;
extern int	screen_width;
extern int	screen_length;

extern int vsync;

extern unsigned char* screen;
extern unsigned char* video;
extern unsigned char* vscreen;

extern unsigned char game_palette[3*256];
extern unsigned char* translucency_table;

extern char *DriverDesc;

extern int GFX_SetMode(int xres, int yres);
extern void VidInfo(void);
extern void GFX_SetPalette(unsigned char* pal);
extern void GFX_GetPalette(void);
extern void GFX_SetPaletteIntensity(int intensity);

// driver interface

extern int (*ShutdownVideo)(int i);
extern int (*ShowPage)(void);

// LFB routines for all driver functions

extern int  LFB_ShowPage(void);
extern void ColorField(int x, int y, byte c);
extern void ClearScreen(void);
extern void MapLine(int x, int y, int yofs, word *map);
extern void TMapLine(int x, int y, int yofs, word *map);

extern void LFB_Blit_8(
	int x, int y, int width, int length, unsigned char* source, int masked, int color_mapped);

extern void CopySpriteZoomClip(int x, int y, int width, int height, int dw, int dh, byte *src);
extern void TCopySpriteZoomClip(int x, int y, int width, int height, int dw, int dh, byte *src);

extern void CopySpriteZoomLucentClip(int x, int y, int width, int height, int dw, int dh, byte *src);
extern void TCopySpriteZoomLucentClip(int x, int y, int width, int height, int dw, int dh, byte *src);
extern void SilhouetteClip(int x, int y, int width, int height, int color, byte *src);
extern void SilhouetteZoomClip(int x, int y, int sw, int sh, int dw, int dh, int color, byte *src);
extern void SilhouetteLucentClip(int x, int y, int width, int height, int color, byte *src);
extern void SilhouetteZoomLucentClip(int x, int y, int sw, int sh, int dw, int dh, int color, byte *src);
extern void TintClip(int x, int y, int width, int height, int color, byte *src);
extern void TintZoomClip(int x, int y, int sw, int sh, int dw, int dh, int color, byte *src);

extern int  GetPixelClip(int x, int y);
extern void SetPixelClip(int x, int y, int color);
extern void SetPixelLucentClip(int x, int y, int color);
extern void HLineClip(int x, int y, int x2, int color);
extern void HLineLucentClip(int x, int y, int x2, int color);
extern void RectFillClip(int x, int y, int x2, int y2, int color);
extern void RectFillLucentClip(int x, int y, int x2, int y2, int color);
extern void VLineClip(int x, int y, int y2, int color);
extern void VLineLucentClip(int x, int y, int y2, int color);
extern void RectClip(int x, int y, int x2, int y2, int color);
extern void RectLucentClip(int x, int y, int x2, int y2, int color);
extern void LineClip(int x, int y, int x2, int y2, int color);
extern void LineLucentClip(int x, int y, int x2, int y2, int color);
extern void CircleFillClip(int x, int y, int radius, int color);
extern void CircleFillLucentClip(int x, int y, int radius, int color);
extern void CircleClip(int x, int y, int radius, int color);
extern void CircleLucentClip(int x, int y, int radius, int color);

extern void LFB_BlitWrap_8(
	quad x, quad y, int width, int length, unsigned char* source, int masked, int color_mapped);

void RotScale(int posx, int posy, quad width, quad height, float angle, float scale, byte *src);
byte *InitMosaicTable(void);
void Mosaic(quad xlevel, quad ylevel, byte *tbl, quad xmin, quad ymin, quad xmax, quad ymax);

#endif // VDRIVER_INC
