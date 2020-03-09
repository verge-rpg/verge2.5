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

// vbe2.cc

/*
	coded by Charles Rector AKA aen
	last updated: 25sep99
*/

// TODO: replace w/ the smaller interface
#include <i86.h>
#include <stdio.h>
//#include <malloc.h>

#include "verge.h"

#include "vbe2.h"
#include "dpmi.h"
#include "linked.h"

int vesa_2_available=0;

//unsigned char* video=0;
MODE_INFO current_mode={0};

static char *mem_models[] = {
	"Text",
	"CGA",
	"Herc",
	"Planar",
	"Packed",
	"Non-Chain4",
	"Direct",
	"YUV",
};

int found_mode=0;	// used w/ vbe_find_mode()

VBE_INFO vbe_info={0};
MODE_INFO mode_info={0};

static linked_list modelist;
struct modei_t
	: public linked_node
{
	MODE_INFO info;

	int compare(void* c)
	{
		return (((modei_t *)c)->info.mode_number > info.mode_number);
	}

	modei_t(MODE_INFO* inf)
	{
		if (inf)
			info=*inf;
	}
};

#define VBE_GET_CONTROLLER_INFO 0x4f00
#define VBE_GET_MODE_INFO 0x4f01
#define VBE_SET_MODE 0x4f02

int vbe_get_info() {
/* get information about capabilities of this card/system */
	//int c=0;
	RM_REGS r={0};
	unsigned long adr=0;

	V_memset(_lowbuf, 0, LOWBUF_SIZE);
	V_memcpy(_lowbuf, "VBE2", 4);

	r.eax=VBE_GET_CONTROLLER_INFO;
	adr=(unsigned long)_lowbuf;
	r.es=(unsigned short)((adr>>4)&0xffff), r.edi=adr&0xf;

	simulate_real_mode_int(0x10,&r);

	if (r.eax != 0x004f) {
		return 0;
	}

// copy info from real-mode mem to our linear vbe_info
	V_memcpy(&vbe_info, _lowbuf, sizeof(VBE_INFO));

	return !strncmp((char *)vbe_info.signature, "VESA", 4);
}

int vbe_get_mode_info(int mode) {
/* get information on a particular mode and copy it into global mode_info member */
	RM_REGS r={0};
	unsigned long adr=0;

	r.eax	=VBE_GET_MODE_INFO;
	adr		=(unsigned long)_lowbuf;
	r.es	=(unsigned short)((adr>>4)&0xffff),r.edi=adr&0xf;
	r.ecx	=mode;

	simulate_real_mode_int(0x10,&r);

	if (r.eax != 0x004f) {
		return 0;
	}

	V_memcpy(&mode_info, _lowbuf, sizeof(MODE_INFO));
	mode_info.mode_number=mode;

	return 1;
}

void* vbe_set_mode(int xres, int yres, int bpp) {
/* attempts to set the video mode. if all was successful, returns start address of video memory.
	on failure, returns NULL. */

	union REGS r={{0}};

// try to find video mode
	if (!vbe_find_mode(xres,yres,bpp)) {
		//printf("Mode %ix%ix%i not found\n", xres,yres,bpp);
		return NULL;
	}
// we found it, get a pointer
	if (!(mode_info.mode_attributes & 0x80)) {
		//printf("LFB unsupported (%ix%ix%i is %s)\n", xres,yres,bpp, mem_models[mode_info.mem_model]);
		return NULL;
	}

	r.w.ax	=VBE_SET_MODE;
	r.w.bx	=(unsigned short)(mode_info.mode_number | (1<<14));

	int386(0x10,&r,&r);

// for user to inferace with
	V_memcpy(&current_mode, &mode_info, sizeof(MODE_INFO));
	//set_clip_rect(0, 0, xres-1, yres-1);
	//set_view_rect(0, 0, xres-1, yres-1);

	// return mapped video pointer
	return dpmi_map_physical((void *)mode_info.phys_base_ptr, xres*yres*bytes_per_pixel());
}

void vbe_find_all_modes() {
/* construct a mode list with info about every mode. could run into troubles if VBE version is toggled
	while program is running, since the mode list is only composed at startup. */

	int n=0;

// destroy old
	while (modelist.head())
	{
		modei_t* m=(modei_t *)modelist.head();
		modelist.unlink((linked_node *)m);
		delete m;
	}

// create new
	while (n<0x3ff) {
		if (vbe_get_mode_info(n++))
			modelist.insert_tail((linked_node *)new modei_t(&mode_info));
	}
}

unsigned long RM_TO_LINEAR(unsigned long adr) {
/* convert a real-mode address to a linear address */
	return ((adr&0xffff0000)>>12)+(adr&0xffff);
}

static int print_mi_mode=0;

// helper
static void _print_mode_info(modei_t* mi) {
/* diagnostic kinda routine to display the info on a mode w/ resolution & bpp etc. */

	if (!mi)
		return;
	MODE_INFO* info=&mi->info;

	printf("%2i³0x%3x³%4i x%5i x%3i³   %2i ³ ", print_mi_mode,
		info->mode_number,
		info->xres, info->yres,
		info->bits_per_pixel,
		info->num_pages);

// if high- or true-color
	if (6 == info->mem_model) {
		char color_bits[33]={0};
		int n, z;
		z=info->red_size+info->green_size+info->blue_size+info->pad_size;

		for (n=0; n<info->red_size; n++)	color_bits[z-info->red_pos-n-1]='R';
		for (n=0; n<info->green_size; n++)	color_bits[z-info->green_pos-n-1]='G';
		for (n=0; n<info->blue_size; n++)	color_bits[z-info->blue_pos-n-1]='B';
		for (n=0; n<info->pad_size; n++)	color_bits[z-info->pad_pos-n-1]='X';

		printf(color_bits);
	} else {
		printf(mem_models[info->mem_model]);
	}

	if (info->mode_attributes & 0x80) {
		printf(" LFB");
	}
	printf("\n");
}

void vbe_print_found_modes() {
/* calls vbe_print_mode_info() for every mode, to produce a listing of general info about all modes */
	printf("\n  %i modes found.\n", modelist.number_nodes());
	printf("\n   Mode   Wid   Hgt  Bits  Pages  Type \n");

	print_mi_mode=0;
	modelist.go_head();
	do	{
		_print_mode_info((modei_t *)modelist.current());
		print_mi_mode++;
		modelist.go_next();
	} while (modelist.current() != modelist.head());
}

int vbe_find_mode(int xres, int yres, int bpp) {
/* search for a mode that matches the given criteria (within pregenerated mode list) */
	if (xres+yres<1) {
		return 0;
	}

	found_mode=0;

	modelist.go_head();
	do	{
		MODE_INFO *info=&(((modei_t *)modelist.current())->info);
		if (info->xres==xres && info->yres==yres && info->bits_per_pixel==bpp) {
			V_memcpy(&mode_info, info, sizeof(MODE_INFO));
			return 1;
		}
		modelist.go_next();
		found_mode++;
	} while (modelist.current() != modelist.head());
	return 0;
}

// helper
static void _print_low(unsigned long adr) {
/* print a string located in <1MB region */
	printf("  %s\n", (char *)RM_TO_LINEAR(adr));
}

void vbe_print_profile() {
/* display VBE version number, video mem avail., and OEM information */

	int n=vbe_info.total_memory<<6;
	printf("VESA %i.%i is present. Video RAM: %i%s\n",
		vbe_info.version>>8, vbe_info.version&0xff,
		n>=1024?n/1024:n, n>=1024?"MB":"KB");

	_print_low(vbe_info.oem_string_pointer);
	_print_low(vbe_info.oem_vendor_name_pointer);
	_print_low(vbe_info.oem_product_name_pointer);
	_print_low(vbe_info.oem_product_rev_pointer);
}

int bytes_per_pixel() {
/* calcs # of bytes necessary to hold data for a single pixel in the current mode */
	return (current_mode.bits_per_pixel+7)/8; // round up
}
