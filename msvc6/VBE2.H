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

// vbe2.h

/*
	coded by Charles Rector AKA aen
	last updated: 25sep99
*/

#ifndef VBE2_INC
#define VBE2_INC

extern int vesa_2_available;

#pragma pack(1)
struct VBE_INFO {
	unsigned char	signature[4];                       // +4
	unsigned short	version;                                 // +2=6
	unsigned long	oem_string_pointer;                      // +4=10
	unsigned char	capabilities[4];                         // +4=14
	unsigned long	video_mode_pointer;                      // +4=18
	unsigned short	total_memory;                            // +2=20
	unsigned short	oem_software_rev;                        // +2=22
	unsigned long	oem_vendor_name_pointer;                 // +4=26
	unsigned long	oem_product_name_pointer;                // +4=30
	unsigned long	oem_product_rev_pointer;                 // +4=34

	unsigned char	reserved[256-34];

	unsigned char	oem_data[256];
};

struct MODE_INFO {
	// Stuff for all VESA versions
	unsigned short	mode_attributes;                         // +2
	unsigned char	win_a_attributes, win_b_attributes;      // +2=4
	unsigned short	win_granularity, win_size;               // +4=8
	unsigned short	win_a_seg, win_b_seg;                    // +4=12
	unsigned long	win_func;                                // +4=16
	unsigned short	bytes_per_scanline;                      // +2=18

	// VESA 1.2+
	unsigned short	xres, yres;                              // +4=22
	unsigned char	x_char_size, y_char_size;                // +2=24
	unsigned char	num_planes;                              // +1=25
	unsigned char	bits_per_pixel;                          // +1=26
	unsigned char	num_banks;                               // +1=27
	unsigned char	mem_model;                               // +1=28
	unsigned char	bank_size_in_k;                          // +1=29
	unsigned char	num_pages;                               // +1=30
	unsigned char	reserved;                                // +1=31

	// direct color fields (required for direct/6 and YUV/7 memory models)
	unsigned char	red_size, red_pos;                       // +2=33
	unsigned char	green_size, green_pos;                   // +2=35
	unsigned char	blue_size, blue_pos;                     // +2=37
	unsigned char	pad_size, pad_pos;                       // +2=39
	unsigned char	direct_color_mode_info;                  // +1=40

	// VESA 2.0+
	unsigned long	phys_base_ptr;                           // +4=44
	unsigned long	offscreen_ptr;                           // +4=48
	unsigned short	offscreen_size;                          // +2=50

	// my stuff
	unsigned long	mode_number;                             // +4=54

	unsigned char	junk[256-54];
};

#pragma pack()

// >>> INTERFACE <<<

extern unsigned char* video;
extern MODE_INFO current_mode;

extern int vbe_get_info();
extern int vbe_get_mode_info(int mode);
extern void vbe_find_all_modes();
extern void vbe_print_found_modes();
extern int vbe_find_mode(int xres, int yres, int bpp);
extern void vbe_print_profile();
extern void* vbe_set_mode(int xres, int yres, int bpp);
extern int bytes_per_pixel();

#endif // VBE2_INC