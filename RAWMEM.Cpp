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
	Raw Memory Module
	coded by aen

	mod log:
	21	December	1999	Created.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "rawmem.h"
#include "verge.h"

#define HUNK 32
#define HUNK_COUNT(x) (((x)+HUNK-1)/HUNK)

static u32 die_on_violate=1;
void die_on_violation(u32 flag) {
	die_on_violate=flag;
}

// ctor
rawmem::rawmem(s32 requested, const char *use) {
	zero_all();
	resize(requested,use);
}
// dtor
rawmem::~rawmem(){
	destroy();
}

u32 rawmem::touched() const {
	return m_touched;
}
void rawmem::touch(u32 rhand) {
	if (rhand>length())
		rhand=length();
	if (rhand>touched())
		m_touched = rhand;
}

void rawmem::untouch() {
	m_touched=0;
}

// used by ctors
void rawmem::zero_all() {
	m_data=0L;
	m_length=m_hunks=m_touched=0;
}
char* rawmem::get_use() const {
	static char temp[RAWMEM_USAGE_STRING_LENGTH +1];
	if (V_strlen((char *)m_use)<1)
		V_strcpy(temp, "*UNKNOWN*");
	else V_strcpy(temp, (char *)m_use);
	return temp;
}

void rawmem::set_use(const char *use) {
	if (!use || V_strlen((char *)use)<1) return;
	else V_strncpy((char *)m_use, (char *)use, 256);
}

void rawmem::become_string(const char* text) {
// resize to accomodate a C-string (zero-terminated), then copy the string in and touch all
	resize(V_strlen((char *)text)+1);
	V_memcpy(get(0,length()), (char *)text, length());
	touch(length());
}

u32 rawmem::length() const { return m_length; }
u32 rawmem::hunks() const { return m_hunks; }
// total amount of memory this rawmem object currently contains
u32 rawmem::bytes_allocated() const { return hunks() * HUNK; }

// free all memory and zero all members; also effectively unmaps a device, if currently mapped
void rawmem::destroy() {
	if (m_data) {
		delete [] m_data;
		m_data = 0L;
	}
	m_length=m_hunks=m_touched=0;
}

// aen <21dec99> Allocation management core.
void rawmem::resize(s32 requested, const char *use) {
	set_use(use);
	//if (requested<1)
	//	requested=1;
	// do we need more blocks?
	if (requested && HUNK_COUNT(requested) > hunks()) {
		m_hunks = HUNK_COUNT(requested);			// reset hunk count
		u8* buf = new u8 [bytes_allocated()];	// allocate a new buffer w/ more needed hunks
		// preservation requested? if so, copy in touched bytes
		if (touched()) {
			V_memcpy(buf, m_data, touched());
		}
		// destroy current buffer, and reassign pointer to new buffer
		delete [] m_data;
		m_data = buf;
	// forgot how this works..
	} else if (requested < length()) {
		touch(requested);
	}

	// zero-out all unused bytes during this resize
	if (touched() < bytes_allocated()) {
		V_memset(m_data+touched(), 0, bytes_allocated()-touched());
	}
	// reset length
	m_length=requested;
}

// reallocate if hunks required to hold length() bytes is less than
// the currently allocated number of blocks
void rawmem::consolidate() {
	// do we really need this many hunks?
	if (HUNK_COUNT(length()) < hunks()) {
		m_hunks = HUNK_COUNT(length());				// reset hunk count
		u8* buf = new u8 [bytes_allocated()];	// allocate a new buffer w/ less hunks
		// any memory in-use? if so, copy it in
		if (touched()) {
			V_memcpy(buf, m_data, touched());
		}
		// destroy current buffer, and reassign pointer to new buffer
		delete [] m_data;
		m_data = buf;
	}
}

void rawmem::report() {
	printf("$ MEM REPORT <%s> # ", get_use());
	printf("<L%d:%d%%> <H%d> <T%d:%d%%>\n",
		length(),
		(length()*100)/bytes_allocated(),
		hunks(),
		touched(),
		(touched()*100)/length()
	);
}

// aen <9nov99> : Yeah baby! The core of all protection.
u8* rawmem::get(s32 n, u32 z) const {
	// all protection for all requests originates here
	if (n<0) {
		if (die_on_violate) {
			n=0-n;
			printf("rawmem$get: %s: prefix violation (offset=%d,count=%d)\n", get_use(), n, z);
			exit(-1);
		}
		n=0;
	// we want things to be okay if length (z) is zero; if we don't check for length,
	// it will trigger the violation code, even though a violation hasn't occurred.
	} else if (z && n+z-1 >= length()) {
		if (die_on_violate) {
			n=n+z-1 -length() +1;
			printf("rawmem$get: %s: suffix violation (offset=%d,count=%d)\n", get_use(), n, z);
			exit(-1);
		}
		n=0;
	}
	return &m_data[n];
}

u32* rawmem::get_u32(s32 n) const { return (u32*)get(n, sizeof(u32)); }
u16* rawmem::get_u16(s32 n) const { return (u16*)get(n, sizeof(u16)); }
u8* rawmem::get_u8(s32 n) const { return get(n, sizeof(u8)); }
u8& rawmem::operator [](s32 n) const { return *get(n, sizeof(u8)); }

void rawmem_fill_u8(rawptr& dest, u32 color, s32 count) {
	if (count<0) return;
	u8* d=dest.get(count);
	if (d)
		V_memset(d,color,count);
	dest.touch(count);
}

void rawmem_xfer(rawptr& dest, rawptr& source, s32 count) {
	if (count<0) return;
	u8 *s=source.get(count);
	u8 *d=dest.get(count);
	if (s && d)
		V_memcpy(d,s,count);
	source.touch(count);
	dest.touch(count);
}

#undef HUNK