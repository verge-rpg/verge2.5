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
	String Module Header
	coded by aen

	mod log:
	21	December	1999	Created.
*/

#ifndef string_inc
#define string_inc

#include "rawmem.h"
#include "linked.h"

class string
	: public linked_node
{
	rawmem m_data;

public:
	int compare(void* n1);

	// !!! NO MEMORY PROTECTION HERE, BE CAREFUL !!!
	char* c_str() const;

	u32 length() const;

	// create & destroy
	void destroy();
	// create via C-string
	void assign(char* text);
	// create via single char
	void assign(char ch);

	// def ctor
	string(char *text = 0);
	// single character ctor
	string(char ch);
	// copy ctor
	string(const string& s);
	// dtor
	~string();

    // assignment op
	string& operator =(const string& s);
	// concatenation
	string& operator+=(const string& s);
	string& operator+ (const string& s);

	u32 touched() const { return m_data.touched(); }

	// indexed char retrieval
	char operator[](s32 n) const;

	// transformers
	string upper();
	string lower();

	// substring extraction; auto-clipping due to operator[]
	string operator()(s32 pos, s32 len); // const;
	// whee!
	string left (s32 len);
	string right(s32 len);
	string mid  (s32 pos, s32 len);

	// locators; return -1 on failure, otherwise the located index
	u32 firstindex(char c) const;
	u32 lastindex (char c) const;

	// equality ops
	bool operator ==(const string& s) const;
	bool operator < (const string& s) const;
	bool operator <=(const string& s) const;
	bool operator > (const string& s) const;
	bool operator >=(const string& s) const;

	void report();
	void consolidate();

	void become_raw(rawmem& rm);
};

#endif // string_inc
