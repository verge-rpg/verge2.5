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

#ifndef KEYBOARD_H
#define KEYBOARD_H

extern unsigned int key_down[256];
//extern unsigned int key_repeats[256];
extern unsigned int key_lastpress;

extern unsigned char	key_queue[256];

enum keydest_t { key_none, key_game, key_console, key_editor };
extern keydest_t key_dest;

extern void Key_SendKeys();
extern unsigned char scantokey[256];
extern unsigned char unpress[256];

/*
extern volatile byte key[128];
extern volatile byte last_pressed;
*/
extern int InitKeyboard(void);
extern void ShutdownKeyboard(void);

#ifdef DJGPP
extern void keyboard_chain(int toggle);
#endif

/* Constants for keyboard scan-codes */

#define SCAN_ESC        0x01
#define SCAN_1          0x02
#define SCAN_2          0x03
#define SCAN_3          0x04
#define SCAN_4          0x05
#define SCAN_5          0x06
#define SCAN_6          0x07
#define SCAN_7          0x08
#define SCAN_8          0x09
#define SCAN_9          0x0a
#define SCAN_0          0x0b
#define SCAN_MINUS      0x0c
#define SCAN_EQUALS     0x0d
#define SCAN_BACKSP     0x0e
#define SCAN_TAB        0x0f
#define SCAN_Q          0x10
#define SCAN_W          0x11
#define SCAN_E          0x12
#define SCAN_R          0x13
#define SCAN_T          0x14
#define SCAN_Y          0x15
#define SCAN_U          0x16
#define SCAN_I          0x17
#define SCAN_O          0x18
#define SCAN_P          0x19
#define SCAN_LANGLE     0x1a
#define SCAN_RANGLE     0x1b
#define SCAN_ENTER      0x1c
#define SCAN_CTRL       0x1d
#define SCAN_A          0x1e
#define SCAN_S          0x1f
#define SCAN_D          0x20
#define SCAN_F          0x21
#define SCAN_G          0x22
#define SCAN_H          0x23
#define SCAN_J          0x24
#define SCAN_K          0x25
#define SCAN_L          0x26
#define SCAN_SCOLON     0x27
#define SCAN_QUOTA      0x28
#define SCAN_RQUOTA     0x29
#define SCAN_LSHIFT     0x2a
#define SCAN_BSLASH     0x2b
#define SCAN_Z          0x2c
#define SCAN_X          0x2d
#define SCAN_C          0x2e
#define SCAN_V          0x2f
#define SCAN_B          0x30
#define SCAN_N          0x31
#define SCAN_M          0x32
#define SCAN_COMA       0x33
#define SCAN_DOT        0x34
#define SCAN_SLASH      0x35
#define SCAN_RSHIFT     0x36
#define SCAN_GREY_STAR  0x37
#define SCAN_ALT        0x38
#define SCAN_SPACE      0x39
#define SCAN_CAPS       0x3a
#define SCAN_F1         0x3b
#define SCAN_F2         0x3c
#define SCAN_F3         0x3d
#define SCAN_F4         0x3e
#define SCAN_F5         0x3f
#define SCAN_F6         0x40
#define SCAN_F7         0x41
#define SCAN_F8         0x42
#define SCAN_F9         0x43
#define SCAN_F10        0x44
#define SCAN_NUMLOCK    0x45
#define SCAN_SCRLOCK    0x46
#define SCAN_HOME       0x47
#define SCAN_UP         0x48
#define SCAN_PGUP       0x49
#define SCAN_GREY_MINUS 0x4a
#define SCAN_LEFT       0x4b
#define SCAN_PAD_5      0x4c
#define SCAN_RIGHT      0x4d
#define SCAN_GREY_PLUS  0x4e
#define SCAN_END        0x4f
#define SCAN_DOWN       0x50
#define SCAN_PGDN       0x51
#define SCAN_INSERT     0x52
#define SCAN_DEL        0x53
#define SCAN_F11        0x57
#define SCAN_F12        0x58

// ***
#define K_ESC 27
#define K_BACKSP 8
#define K_TAB 9
#define K_ENTER 13
#define K_CTRL 200
#define K_SHIFT 201
#define K_ALT 202
#define K_CAPS 203

#define K_F1 204
#define K_F2 205
#define K_F3 206
#define K_F4 207
#define K_F5 208
#define K_F6 209
#define K_F7 210
#define K_F8 211
#define K_F9 212
#define K_F10 213
#define K_F11 214
#define K_F12 215

#define K_NUMLOCK 216
#define K_SCRLOCK 217

#define K_HOME 218
#define K_UP 219
#define K_PAGEUP 220
#define K_PAGEDN 221
#define K_DOWN 222
#define K_END 223
#define K_LEFT 224
#define K_RIGHT 225
#define K_INS 226
#define K_DEL 227
// ***

#endif
