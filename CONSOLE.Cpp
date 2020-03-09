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
// ³                      Command Console module                         ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

/*
	mod log:

	<aen>
	28	Decemeber	1999	Major revamp.
*/

#include "verge.h"

#define CONSOLE_TEXT_LINES 100
#define CONSOLE_LAST_LINES 25

byte	cpu_watch	=0;
byte	cpubyte		=0;

class console_command_t
	: public linked_node
{
	string m_name;
	void (*m_execute)();

public:
	int compare(void* c)
	{
		return ( ((console_command_t *)c)->m_name > m_name );
	}

	console_command_t(string s, void (*e)())
	: m_name(s), m_execute(e) {}

	string name() const { return m_name; }
	void execute()
	{
		if (m_execute)
			m_execute();
	}
};
static linked_list concmds;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION /////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

static int console_processing=0;

static byte*	consolebg	=0;		// console background image

static linked_list consoletext;

static string glob_cmd; // TODO: make this static; ramifications w/ VERGE.CC

static linked_list lastcmds;
static char cmdpos=0;		// last-command memory

static linked_list* args=0;

static char cursor=1;	// flag on/off cursor visible
static int cswtime=0;	// cursor switch time.

// aen <24dec99> Changed to fixed point.
static int conlines=0;	// Number of visible lines

static int backtrack=0;

static int allowconsole=1;
static int consoleoverride=0;

int Con_Width() { return true_screen_width; }
int Con_Length() { return (120.0/200.0)*true_screen_length+2; }
int Con_Lines() { return Con_Length()/Font_GetLength(0)-1; }
int Con_CharsPerLine() { return Con_Width()/Font_GetWidth(0)-2; }

int Con_NumArgs()
{
	return args ? args->number_nodes() : 0;
}
string Con_GetArg(int x) {
	if (!args
	|| x<0 || x>=args->number_nodes())
		return "";
	return *(string *)args->get_node(x+1);
}

int Con_BottomEdge() { return conlines; }
void Con_SetViewablePixelRows(int x)
{
	if (x<0)
		x=0;
	if (x>Con_Length())
		x=Con_Length();

	conlines=x;
}

int Con_ViewLines() { return Con_Length()/Font_GetLength(0)+2; }

int Con_IsConsoleAllowed() { return allowconsole; }
void Con_AllowConsole(int allow) { allowconsole=allow; }

int Con_IsOverriden() { return consoleoverride; }
void Con_SetOverride(int override) { consoleoverride=override; }

/*
static byte key_ascii_tbl[128] =
{
  0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a', 's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  0,   0,   92,  'z', 'x', 'c', 'v',
  'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,
  3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,
  0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,
  13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0
};

static byte key_shift_tbl[128] =
{
   0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 126, 126,
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 126, 0,   'A', 'S',
   'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34,  0,   0,   '|', 'Z', 'X', 'C', 'V',
   'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   1,   0,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,
   0,   0,   1,   127, 0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0
};
*/

static void Con_AddCommand(string name, void (*execute)())
{
	concmds.insert((linked_node *)new console_command_t(name, execute));
}

void Console_Draw(void)
{
	Render();
	if (consolebg)
	{
		LFB_BlitZoom(
			0, -Con_Length()+Con_BottomEdge(),
			320, 120,
			Con_Width(), Con_Length(),
			consolebg, 0, 1);
	}
	else
	{
		int y = -Con_Length()+Con_BottomEdge();
		LFB_BlitRect(
			0, y,
			Con_Width(), y+Con_Length()-1,
			0,
			1, 0);
	}


	// write console text
	if (consoletext.number_nodes()>0)
	{
		int row=Con_BottomEdge() -1 -(Font_GetLength(0)*(2 +(backtrack?1:0)));
		int n=Con_Lines() -(backtrack?1:0);

		consoletext.go_tail();
		for (int x=0; x<backtrack; x++)
			consoletext.go_prev();
		do	{
			Font_GotoXY(1, row);
			Font_Print(0, *(string *)consoletext.current());
			row -= Font_GetLength(0);

			consoletext.go_prev();
		} while (--n
		&& consoletext.current() != consoletext.tail());
	}

	if (backtrack)
	{
		Font_GotoXY(1, Con_BottomEdge() -1 -(Font_GetLength(0)*2));
		while (Font_GetX()+(Font_GetWidth(0)*3) < true_screen_width)
		{
			Font_Print(0, "^   ");
		}
	}

	// paint command prompt & cursor
	Font_GotoXY(1, Con_BottomEdge() -1 -Font_GetLength(0));
	int startx=0;
	if (glob_cmd.length() > Con_CharsPerLine())
	{
		Font_Print(0, cursor?"<":" ");
		startx=glob_cmd.length()-Con_CharsPerLine();
	}
	else
	{
		Font_Print(0, "]");
	}
	Font_Print(0, glob_cmd.mid(startx,Con_CharsPerLine()));
	if (systemtime>=cswtime)
	{
		cursor ^= 1;
		cswtime = systemtime+40;
	}
	if (cursor)
		Font_Print(0, "_");
}

void ParseCommand(string s)
{
	// breaks the command string into arguements and stuff. (in cmd2)
	int start, count;

	start=0;
	count=0;

	if (args)
		delete args;
	args = new linked_list;

	while (count<s.length())
	{
		if (' ' >= s[count])
		{
			while (count<s.length() && ' ' >= s[count])
				count++;
		}
		if (count<s.length())
		{
			start=count;
			while (count<s.length() && ' ' < s[count])
				count++;

			args->insert_tail((linked_node *)new string(s(start,count-start)));
		}
	}
}

void ConKey_Tab()
{
	int len=glob_cmd.length();
	if (!len)
	{
		return;
	}

	concmds.go_head();
	do	{
		console_command_t* c=(console_command_t *)concmds.current();
		if (glob_cmd.upper()(0,len) == c->name().upper()(0,len))
		{
			glob_cmd=c->name().lower();
			break;
		}
		concmds.go_next();
	} while (concmds.current() != concmds.head());
}

void ConKey_Type(int k)
{
	// printable characters
	if (k>31 && k<128)
	{
		if (glob_cmd.length() < 256)
			glob_cmd+=(char)k;
	}
}

void Con_Key(int key)
{
	switch (key)
	{
	// done w/ console
	case '`':
		console_processing=0;
		break;

	case K_HOME:
		backtrack = consoletext.number_nodes()-1;
		break;

	case K_END:
		backtrack = 0;
		break;

	case K_PAGEUP:
		backtrack+=2;
		if (backtrack > consoletext.number_nodes()-1)
			backtrack = consoletext.number_nodes()-1;
		break;

	case K_PAGEDN:
		backtrack-=2;
		if (backtrack < 0)
			backtrack = 0;
		break;

	case K_UP:
		if (cmdpos<lastcmds.number_nodes())
			glob_cmd = *(string *)lastcmds.get_node(++cmdpos);
		break;

	case K_DOWN:
		if (cmdpos)
			glob_cmd = --cmdpos ? *(string *)lastcmds.get_node(cmdpos) : "";
		break;

	case K_LEFT:
	case K_BACKSP:
		if (glob_cmd.length())
		{
			glob_cmd=glob_cmd.left(glob_cmd.length()-1);
		}
		break;

	case K_TAB:
		ConKey_Tab();
		break;

	case K_ENTER:
		Console_SendCommand(glob_cmd);
		break;

	default:
		ConKey_Type(key);
		break;
	}
}

#include "conlib.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERFACE //////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

void Console_Init(void)
{
	int		slot	=0;

	Logp("Initialize console.");

	slot	=Font_Load("system.fnt");
	if (slot != 0)
	{
		Sys_Error("Console_Init: system.fnt in wrong slot (reported %d).\n", slot);
	}

	Con_AddCommand("CONSOLEBG",		Conlib_ConsoleBG);
	Con_AddCommand("LISTMOUNTS",	Conlib_ListMounts);
	Con_AddCommand("PACKINFO",		Conlib_PackInfo);
	Con_AddCommand("LISTCMDS",		Conlib_ListCmds);
	Con_AddCommand("CD_PLAY",		Conlib_CD_Play);
	Con_AddCommand("CD_STOP",		Conlib_CD_Stop);
	Con_AddCommand("CD_OPEN",		Conlib_CD_Open);
	Con_AddCommand("CD_CLOSE",		Conlib_CD_Close);
	Con_AddCommand("EXIT",			Conlib_Exit);
	Con_AddCommand("VID_MODE",		Conlib_Vid_Mode);
	Con_AddCommand("CPU_USAGE",		Conlib_Cpu_Usage);
	Con_AddCommand("MOUNT",			Conlib_Mount);
	Con_AddCommand("MAP",			Conlib_Map);
	Con_AddCommand("VER",			Conlib_Ver);
	Con_AddCommand("BROWSETILES",	Conlib_BrowseTiles);
	Con_AddCommand("WARP",			Conlib_Warp);
	Con_AddCommand("CAMERATRACKING",Conlib_CameraTracking);
	Con_AddCommand("RSTRING",		Conlib_RString);
	Con_AddCommand("SHOWOBS",		Conlib_ShowObs);
	Con_AddCommand("PHANTOM",		Conlib_Phantom);
	Con_AddCommand("ENTITYSTAT",	Conlib_EntityStat);
	Con_AddCommand("ACTIVEENTS",	Conlib_ActiveEnts);
	Con_AddCommand("ENTITY",		Conlib_Entity);
	Con_AddCommand("CURPOS",		Conlib_CurPos);
	Con_AddCommand("PLAYERSPEED",	Conlib_PlayerSpeed);
	Con_AddCommand("SPEEDDEMON",	Conlib_SpeedDemon);
	Con_AddCommand("RV",			Conlib_RV);
	Con_AddCommand("SV",			Conlib_SV);
	Con_AddCommand("PLAYER",		Conlib_Player);
	Con_AddCommand("SPAWNENTITY",	Conlib_SpawnEntity);
	Con_AddCommand("SHOWZONES",		Conlib_ShowZones);

	Con_AddCommand("EDIT",			Conlib_EditScript);

// we can send this command... *after* we've added the commands. d'oh.
	Console_SendCommand("consolebg console.gif");

	LogDone();
}

void Console_Printf(string str)
{
	if (consoletext.number_nodes() >= CONSOLE_TEXT_LINES)
	{
		string* s=(string *)consoletext.head();
		consoletext.unlink((linked_node *)s);
		delete s;
	}
	consoletext.insert_tail((linked_node *)new string(str));

	backtrack=0;
}

void Console_SendCommand(string cmd)
{
	if (!concmds.number_nodes()) return;

	ParseCommand(cmd.upper());
	if (Con_NumArgs()<1)
	{
		//cmd="";
		glob_cmd="";
		return;
	}

	// always show what was typed
	Console_Printf(va("]%s", cmd.c_str()));

	// starting link
	int found=0;
	concmds.go_head();
	do  {
		console_command_t* c;

		c=(console_command_t *)concmds.current();
		if (c->name() == Con_GetArg(0))
		{
			c->execute();
			found=1;
			break;
		}
		concmds.go_next();
	} while (concmds.current() != concmds.head());

	// if we couldn't find anything, let user know
	if (!found)
	{
		Console_Printf("*** unrecognized command ***");
	}

	lastcmds.insert_head((linked_node *)new string(cmd));
	if (lastcmds.number_nodes() > CONSOLE_LAST_LINES)
	{
		string* s=(string *)lastcmds.tail();
		lastcmds.unlink((linked_node *)s);
		delete s;
	}

// globals
	cmdpos=0;
	glob_cmd="";
}

void Console_Activate()
{
	if (!allowconsole && !consoleoverride) return;

	int tag, r;
	tag=r=0;

	Con_SetViewablePixelRows(r);

	console_processing=1;
	while (console_processing)
	{
		tag=systemtime;

		Console_Draw();
		ShowPage();

		Key_SendKeys();

		CheckMessages();

		tag=systemtime-tag;
		while (tag>0)
		{
			tag--;

			Con_SetViewablePixelRows(r*r);
			if (r*r<Con_Length())
				r++;
		}
	}

// blech
// TODO: get all main render loops for vc editor, map renders, this console, etc. into a single
// routine, just like input all goes thru Key_SendKeys.
	while (conlines > 1)
	{
		tag=systemtime;

		Console_Draw();
		ShowPage();

		CheckMessages();

		tag=systemtime-tag;
		while (tag)
		{
			tag--;

			Con_SetViewablePixelRows(r*r);
			if (r>0)
				r--;
		}
	}

	timer_count=0;
}
