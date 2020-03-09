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
// ³                          Keyboard module                            ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

//#define KEYBOARD_H

#define  WIN32_LEAN_AND_MEAN
#include "dinput.h"
#include "verge.h"

// ================================= Data ====================================
//unsigned char padding[1024];

unsigned char scantokey[256]=
{
	// 2
	0, K_ESC,
	// 12
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	// 16
	'-', '=', K_BACKSP, K_TAB,
	// 26
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	// 30
	'[', ']', K_ENTER, K_CTRL,
	// 39
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
	// 44
	';', '\'', '`', K_SHIFT, '\\',
	// 51
	'z', 'x', 'c', 'v', 'b', 'n', 'm',
	// 59
	',', '.', '/', K_SHIFT, '*', K_ALT, ' ', K_CAPS,
	// 69
	K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10,
	// 75
	K_NUMLOCK, K_SCRLOCK, K_HOME, K_UP, K_PAGEUP, '-',
	// 84
	K_LEFT, 0/*key-5*/, K_RIGHT, '+', K_END, K_DOWN, K_PAGEDN, K_INS, K_DEL,
	// 86
	0, 0, 0, K_F11, K_F12,
	// 100
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 114
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  128
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  144
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  176
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  192
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  208
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  224
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  240
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  256
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

unsigned int key_down[256];
static unsigned int key_repeats[256];
unsigned int key_lastpress=0;

keydest_t key_dest=key_none;

unsigned char unpress[256];

static unsigned char keyshift[256];
static unsigned int shift_down=0;

unsigned char	head=0;
unsigned char	tail=0;
unsigned char	key_queue[256];

LPDIRECTINPUT dinput;
DIPROPDWORD dipdw;
LPDIRECTINPUTDEVICE  di_keyb;
char key[256];
unsigned char last_pressed;

int key_timer, key_repeater, repeatedkey;

void ShutdownKeyboard(void);

extern HWND hMainWnd;
extern HINSTANCE hMainInst;

// ***************************** Code *****************************

int InitKeyboard()
{
	HRESULT hr;
	int n;
   
	hr = DirectInputCreate(hMainInst, DIRECTINPUT_VERSION, &dinput, NULL);
	if (FAILED(hr))
	{
		err("DirectInputCreate");
		return 0;
	}

	hr = dinput -> CreateDevice(GUID_SysKeyboard, &di_keyb, NULL);   
	if (FAILED(hr))
	{
		err("dinput CreateDevice");
		return 0;
	}

	hr = di_keyb -> SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr))
	{
		err("dinput SetDataFormat");
		return 0;
	}

	hr = di_keyb -> SetCooperativeLevel(hMainWnd,DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))
	{
		err("sinput SetCooperativeLevel");
		return 0;
	}

	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = 64;

	hr = di_keyb -> SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr))
	{
		err("Set buffer size");
		return 0;
	}

	hr = di_keyb -> Acquire();
	if (FAILED(hr))
	{
		err("Acquire");
		return 0;
	}   

	for (n=0 ; n<256 ; n++)
		keyshift[n] = (byte)n;
	for (n='a' ; n<='z' ; n++)
		keyshift[n] = (byte)(n - 'a' + 'A');
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	V_memset(key_repeats, 0, sizeof(key_repeats));
	V_memset(unpress, 0, sizeof(unpress));
	V_memset(key_down, 0, sizeof(key_down));

	return 1;
}

void ShutdownKeyboard(void)
{
   if (di_keyb)
   {
      di_keyb -> Unacquire();
      di_keyb -> Release();
      di_keyb = NULL;
   }
   if (dinput)
   {
      dinput -> Release();
      dinput = NULL;
   }
}

void Key_Event(int key, int press)
{
	if (key_down[K_ALT] && key=='x' && press)
	{
		Sys_Error("Exiting: ALT-X pressed.");
	}

	//key_down[key]=press;

	if (!press)
	{
		key_repeats[key]=0;
		unpress[key]=0;
	}

	//key_lastpress=key;

	if (press)
	{
		if (unpress[key])
			return;

		key_repeats[key]++;
		// don't autorepeat console summoner
		if (key_repeats[key]>1)
		{
			if ('`'==key
			// or console buttons
			//|| kb1==key || kb2==key || kb3==key || kb4==key
			)
			{
				return;
			}
		}
	}

	// these care about up/down
	if (kb1==key)
		b1=press;
	if (kb2==key)
		b2=press;
	if (kb3==key)
		b3=press;
	if (kb4==key)
		b4=press;

	if (K_UP==key)
		up=press;
	if (K_DOWN==key)
		down=press;
	if (K_LEFT==key)
		left=press;
	if (K_RIGHT==key)
		right=press;

	if (key == K_SHIFT)
		shift_down=press;

	if (!press) return;

	if (bindarray[key])
	{
		HookKey(bindarray[key]);
	}

	// keys to game do not care about case
	if (key_dest == key_game)
	{
		Key_Game(key);
		return;
	}

	if (shift_down)
		key=keyshift[key];

	switch (key_dest)
	{
	case key_console:
		Con_Key(key);
		break;

	case key_editor:
		V2SE_Key(key);
		break;
	}
}

void Key_SendKeys()
{
	HRESULT hr;
	int y=0;
	DIDEVICEOBJECTDATA rgod[64];
	DWORD cod=64, iod;
	
	CheckMessages();
	//ReadMouse();
	hr = di_keyb -> GetDeviceData(sizeof(DIDEVICEOBJECTDATA),rgod, &cod, 0);	//retrive data
	if(hr!=DI_OK&&hr!=DI_BUFFEROVERFLOW)
	{
		hr = di_keyb -> Acquire();
		if(!SUCCEEDED(hr))
			if(hr==DIERR_OTHERAPPHASPRIO) goto nevermind;
	}
	else if(cod>0&&cod<=64)
	{
		for(iod=0;iod<cod;iod++)
		{
			key_lastpress=rgod[iod].dwData;
			key_down[scantokey[rgod[iod].dwOfs]]=rgod[iod].dwData;
			Key_Event(scantokey[rgod[iod].dwOfs],rgod[iod].dwData);
		}
	}
nevermind:;
}


