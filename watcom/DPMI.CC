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

// dpmi.cc

/*
	coded by Charles Rector AKA aen
	last updated: 25sep99
*/

#include <i86.h>
#include "dpmi.h"
#include "verge.h"

char *_lowbuf=0;

static void __free_lowbuf() {
	//printf("freeing _lowbuf...");
	dpmi_real_free(_lowbuf);		// free low-mem xfer buffer
}
void __setup_lowbuf() {
	_lowbuf=(char *)dpmi_real_malloc(LOWBUF_SIZE);	// allocate xfer buffer
	if (!_lowbuf) {
		Sys_Error("Unable to allocate low-memory.\n");
	}
	atexit(__free_lowbuf);
}

void simulate_real_mode_int(int intno, struct RM_REGS* in) {
/* simulates a real-mode interrupt through DPMI */
	union REGS r={{0}};
	struct SREGS sr={0};

	if (!in) {
		return;
	}

	r.w.ax=DPMI_SIM_REAL_MODE_INT;
	r.w.bx=(short)(intno&0xff);	// BL=real-mode int no., BH=flags (reserved, must be zero)
	r.w.cx=0;			// CX=number of words to copy from protected mode to real-mode stack
	sr.es=FP_SEG(in);	// ES:EDI=selector:offset of real-mode register data
	r.x.edi=FP_OFF(in);

	int386x(DPMI_INT,&r,&r,&sr);
	// on success: carry flag=clear, ES:EDI=offset of modified real-mode register data
	// on failure: carry flag=set, AX=error code
	//		error codes:
	//		0x8012=linear memory unavailable (stack)
	//		0x8013=physical memory unavailable (stack)
	//		0x8014=backing store unavailable (stack)
	//		0x8021=invalid value (CX too large)

	// failure diagnostics
	if (r.x.cflag&1) {
		printf("simulate_real_mode_int: ");
		r.x.eax&=0xffff;
		if (0x8012==r.x.eax) {
			printf("linear memory unavailable (stack)");
		} else if (0x8013==r.x.eax) {
			printf("physical memory unavailable (stack)");
		} else if (0x8014==r.x.eax) {
			printf("backing store unavailable (stack)");
		} else if (0x8021==r.x.eax) {
			printf("invalid value (CX too large)");
		}
		exit(0);
	}
}

// opted for real-mode simulation because a selector doesn't need to be tracked. ^_^
void* dpmi_real_malloc(int size) {
/* allocate memory within <1MB region */
	struct RM_REGS r={0};

	r.eax=0x4800;					// AH=0x48
	r.ebx=(size+15)/16;				// BX=number of paragraphs to allocate

	simulate_real_mode_int(0x21,&r);
	// on success: carry flag=clear, AX=segment of allocated block
	// on failure: carry flag=set, AX=error code (0x07/0x08), BX=size of largest avail. block
	//		error codes:
	//		0x07=memory control block destroyed
	//		0x08=insufficient memory

	// failure diagnostics
	if (r.flags&1) {
		printf("dpmi_real_malloc: ");
		r.eax&=0xffff;	// get AX
		if (0x07==r.eax) {
			printf("memory control block destroyed\n");
		} else if (0x08==r.eax) {
			printf("insufficient memory\n");
		}
		return NULL;
	}

	return (void *)((r.eax&0xffff)<<4);
}

// opted for real-mode simulation because a selector doesn't need to be tracked. ^_^
void dpmi_real_free(void* ptr) {
/* free memory allocated within <1MB region */
	struct RM_REGS r={0};

	if (!ptr) return;	// bogus call

	r.eax=0x4900;
	r.es=(unsigned short)(((unsigned long)ptr)>>4);	// ES=segment of block to free

	simulate_real_mode_int(0x21,&r);
	// on success: carry flag=clear
	// on failure: carry flay=set, AX=error code (0x07/0x09; never returns 0x07 in DOS 2.1+?)
	//		error codes:
	//		0x07=memory control block destroyed
	//		0x09=memory block address invalid

	// failure diagnostics
	if (r.flags&1) {
		printf("dpmi_real_free: ");
		r.eax&=0xffff;	// get AX
		if (0x07==r.eax) {
			printf("memory control block destroyed\n");
		} else if (0x09==r.eax) {
			printf("memory block address invalid\n");
		}
		exit(0);
	}
}

void* dpmi_map_physical(void* physical, int bytes) {
/* converts a physical address -> linear address, allowing device access via the linear address */
	//unsigned long bytes=400*300*4; //1024*1024*8;
	union REGS r={{0}};

	r.x.eax=0x0800;
	r.x.ebx=((unsigned long)physical)>>16;		// BX:CX = physical address
	r.x.ecx=((unsigned long)physical)&0xffff;
	r.w.si=(unsigned short)(bytes>>16);							// SI:DI = size of region to map (in bytes)
	r.w.di=(unsigned short)(bytes&0xffff);

	int386(0x31,&r,&r);
	// on success: carry flag=clear, BX:CX=linear address
	// on failure: carry flag=set, AX=error code (0x8003=system integrity/0x8021=invalid value)

	// failure diagnostics
	if (r.w.cflag&1) {
		printf("dpmi_map_physical: ");
		r.x.eax&=0xffff;	// get AX
		if (0x8003==r.x.eax) {
			printf("system integrity");
		} else if (0x8021==r.x.eax) {
			printf("invalid value");
		}
	}

	return (void *)(r.w.cx+(r.w.bx<<16));
}

void dpmi_unmap_physical(void* linear) {
/* releases mapping of physical -> linear that was previously obtained via dpmi_map_physical() */
	union REGS r={{0}};

	r.x.eax=0x0801;
	r.x.ebx=((unsigned long)linear)>>16;		// BX:CX = linear address
	r.x.ecx=((unsigned long)linear)&0xffff;

	int386(0x31,&r,&r);
	// on success: carry flag=clear
	// on failure: carry flag=set, AX=error code (0x8025=invalid linear address)

	// Don't even need to check this???
	/*
	// failure diagnostics
	if (r.w.cflag&1
	&& r.x.eax!=0x0801)	{ // kludge.
		printf("dpmi_unmap_physical: ");
		r.x.eax&=0xffff;	// get AX
		if (0x8025==r.x.eax) {
			printf("invalid linear address");
		}
	}
	*/
}