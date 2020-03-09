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
	String Module
	coded by aen

	mod log:
	21	December	1999	Created.
*/

#include <ctype.h>
//#include <string.h>

//#include "rawstr.h"
#include "verge.h"

int string::compare(void* c)
{
        return (*((string *)c) > *this);
}

// !!! NO MEMORY PROTECTION HERE, BE CAREFUL !!!
char* string::c_str() const {
	return (char*)m_data.get(0,m_data.length());
}

u32 string::length() const {
	return m_data.length() > 0 ? m_data.length() -1 : 0;
}

// create & destroy
void string::destroy() {
	m_data.destroy();
}
// create via C-string
void string::assign(char* text) {
	// exit gracefully, if no string
	if (!text) {
		return;
	}
	// free mem & resize
	destroy();
	// copy the string in
	m_data.become_string(text);
}
// create via single char
void string::assign(char ch) {
	char text[2] = {ch,'\0'};

	assign(text);
}

// def ctor
string::string(char *text)
: m_data(0,"string") {
	assign(text ? text : "");
}
// single character ctor
string::string(char ch)
: m_data(0,"string") {
	assign(ch);
}
// copy ctor
string::string(const string& s)
: m_data(0,"string") {
	*this = s;
}
// dtor
string::~string() {
	destroy();
}

// assignment op
string& string::operator=(const string& s) {
	if (&s != this) {
		assign((char *)s.m_data.get(0,s.m_data.length()));
	}
	return *this;
}
// concatenation
string& string::operator+=(const string& s) {
	int z = length();

	m_data.resize(z + s.length() +1);

	/*
	extern void Log(char* message);
	char shit[1024];
	sprintf(shit, "+= B4 %s", (char *)m_data.get(0,1));
	Log(shit);
	*/

	//V_memcpy(m_data.m_data+z, s.m_data, s.length());
	//m_data.m_data[z]='\0';
	rawptr source((rawmem *)&s.m_data), dest(&m_data,z);
	rawmem_xfer(dest, source, s.length());
	//(*this)[z+s.length()]='\0';
	m_data[z+s.length()]='\0';
	/*
	sprintf(shit, "+= A %s", (char *)m_data.get(0,1));
	Log(shit);
	*/

	return *this;
}
string& string::operator+(const string& s) {
	return *this += s;
}

// indexed char retrieval
char string::operator[](s32 n) const {
	return (n<0 || n>=length()) ? (char)0 : c_str()[n];
}

// transformers
string string::upper() {
	string s = *this;
	strupr((char *)s.m_data.get(0,s.m_data.length()));
	return s;
}
string string::lower() {
	string s = *this;
	strlwr((char *)s.m_data.get(0,s.m_data.length()));
	return s;
}

// substring extraction; auto-clipping due to operator[]
string string::operator()(s32 pos, s32 len) { //const {
	string s;

	// bogus length
	if (len<1 || pos>=length()) {
		return s;
	}

	// clipping
	if (pos<0) {
		len+=pos;
		pos=0;
	}
	if (pos+len>=length())
		len=length()-pos+1;

	// blap!
	s.m_data.resize(len +1);
	rawptr source(&m_data,pos), dest(&s.m_data,0);
	rawmem_xfer(dest, source, len);

	s.m_data[len]='\0';

	/*
	// slow method
	int n = 0;
	while (n < len) {
		s += operator[](pos + n);
		n++;
	}
	*/
	return s;
}
// whee!
string string::left(s32 len) { return operator()(0, len); }
string string::right(s32 len) { return operator()(length()-len, len); }
string string::mid(s32 pos, s32 len) { return operator()(pos, len); }

#pragma off (unreferenced);
// locators; return -1 on failure, otherwise the located index
u32 string::firstindex(char c) const {
	/*
	char *found = strchr(c_str(), c);
	return found ? found - c_str() : -1;
	*/
	return 0;
}
u32 string::lastindex(char c) const {
	/*
	char *found = strrchr(c_str(), c);
	return found ? found - c_str() : -1;
	*/
	return 0;
}
#pragma on (unreferenced);

// equality ops
bool string::operator==(const string& s) const { return V_strcmp(c_str(), s.c_str()) == 0; }
bool string::operator< (const string& s) const { return V_strcmp(c_str(), s.c_str()) <  0; }
bool string::operator<=(const string& s) const { return V_strcmp(c_str(), s.c_str()) <= 0; }
bool string::operator> (const string& s) const { return V_strcmp(c_str(), s.c_str()) >  0; }
bool string::operator>=(const string& s) const { return V_strcmp(c_str(), s.c_str()) >= 0; }

void string::report() {	m_data.report(); }
void string::consolidate() { m_data.consolidate(); }

void string::become_raw(rawmem& rm) {
	u32 z=V_strlen((char *)rm.get(0,rm.length()));
	m_data.resize(z+1);

	rawptr source(&rm), dest(&m_data);
	rawmem_xfer(dest, source, z);

	m_data[z]='\0';
}
