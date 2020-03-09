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

//int ClipOn=1;
int LucentOn=0;

void vc_UnPress()
{
	int		n;

	n=ResolveOperand();

	UnPress(n);
}

void vc_Exit_()
{
	string	message;

	message=ResolveString();

	Sys_Error(message.c_str());
}

void vc_Message()
{
	string	message;
	int		n;

	message=ResolveString();
	n=ResolveOperand();

	Message_Send(message, n);
}

void vc_Malloc()
{
	int		n;

	n=ResolveOperand();

	vcreturn=(int)valloc(n, "vcreturn", OID_TEMP);

	Log(va("VC allocating %u bytes, ptr at 0x%08X.", n, vcreturn));

	if (!vcreturn)
		Message_Send("Warning: VC failed malloc", 750);
}

void vc_Free()
{
	int		ptr;

	ptr=ResolveOperand();

	vfree((void *)ptr);

	Log(va("VC freeing allocated heap at 0x%08X.", ptr));
}

void vc_pow()
{
	int		a, b;

	a=ResolveOperand();
	b=ResolveOperand();

	vcreturn=pow(a, b);
}

void vc_loadimage()
{
	string	filename;

	filename=ResolveString();

	vcreturn=(int)Image_LoadBuf(filename.c_str());
	if (!vcreturn)
	{
		Sys_Error("vc_loadimage: %s: unable to open", filename.c_str());
	}
}

void vc_copysprite()
{
	int		x, y, width, length;
	int		ptr;

	x		=ResolveOperand();
	y		=ResolveOperand();
	width	=ResolveOperand();
	length	=ResolveOperand();
	ptr		=ResolveOperand();

	LFB_Blit_8(x, y, width, length, (byte *)ptr, 0, LucentOn && translucency_table);
}

void vc_tcopysprite()
{
	int		x, y, width, length;
	int		ptr;

	x		=ResolveOperand();
	y		=ResolveOperand();
	width	=ResolveOperand();
	length	=ResolveOperand();
	ptr		=ResolveOperand();

	LFB_Blit_8(x, y, width, length, (byte *)ptr, 1, LucentOn && translucency_table);
}

void vc_EntitySpawn()
{
	int		tilex, tiley;
	string	chr_filename;

	tilex=ResolveOperand();
	tiley=ResolveOperand();
	chr_filename=ResolveString();

	vcreturn=AllocateEntity(tilex, tiley, chr_filename.c_str());
}

void vc_SetPlayer()
{
	int		n;

	n=ResolveOperand();
	if (n<0 || n>=entities)
	{
		Sys_Error("vc_SetPlayer: entity index out of range (attempted %d)", n);
	}

	player=&entity[n];
	playernum=(byte)n;

//	entity[n].movecode=0;
	entity[n].moving=0;
}

void vc_Map()
{
	hookretrace=0;
	hooktimer=0;
	kill=1;

	startmap=ResolveString();
}

void vc_LoadFont()
{
	string	filename;

	filename=ResolveString();

	vcreturn=Font_Load(filename);
}

void vc_PlayFLI()
{
	Log("vc_PlayFLI disabled.");

	ResolveString();	// FLI filename

	/*
	string fli_filename=ResolveString();

	BITMAP flibuf;
	flibuf.w=screen_width;
	flibuf.h=screen_length;
	flibuf.data=screen;

	VFILE* f=vopen(fli_filename.c_str());
	if (!f)
	{
		Sys_Error("vc_PlayFLI: could not open %s.", fli_filename.c_str());
	}

	unsigned int n=filesize(f);
	byte* data=(byte *)valloc(n, "vc_PlayFLI:data", 0);
	if (!data)
	{
		vclose(f);
		Sys_Error("vc_PlayFLI: Not enough memory to play FLI.");
	}
	vread(data, n, f);
	vclose(f);

	play_memory_fli(data, &flibuf, 0, ShowPage);

	vfree(data);

	timer_count=0;
	set_intensity(63);
	*/
}

void vc_PrintString()
{
	string	text;
	int		font_slot;

	font_slot=ResolveOperand();
	text=ResolveString();

	Font_Print(font_slot, text);
}

void vc_LoadRaw()
{
	string	raw_filename;
	VFILE*	vf;
	int		n;
	char*	ptr;

	raw_filename	=ResolveString();

	vf=vopen(raw_filename.c_str());
	if (!vf)
	{
		Sys_Error("vc_LoadRaw: could not open file %s", raw_filename.c_str());
	}
	n=filesize(vf);
	ptr=(char *)valloc(n, "LoadRaw:t", OID_VC);
	if (!ptr)
	{
		Sys_Error("vc_LoadRaw: memory exhausted on ptr");
	}
	vread(ptr, n, vf);
	vclose(vf);

	vcreturn=(int)ptr;
}

// TODO: rename the layer[] and layers[] arrays less-confusingly. ;P
void vc_SetTile()
{
	int		x, y, lay, value;

	x		=ResolveOperand();
	y		=ResolveOperand();
	lay		=ResolveOperand();
	value	=ResolveOperand();

// ensure all arguments are valid
	if (x<0 || y<0)
		return;
	if (lay==6 || lay==7)
	{
		if (x>=layer[0].sizex || y>=layer[0].sizey)
			return;
	}
	else
	{
		if ((lay>=0 && lay<6)
		&& (x>=layer[lay].sizex || y>=layer[lay].sizey))
		{
			return;
		}
	}

// determine action
	switch (lay)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		layers[lay][y*layer[lay].sizex+x] = (short)value;
		break;

	case 6:
		obstruct[y*layer[0].sizex+x] = (byte)value;
		break;
	case 7:
		zone[y*layer[0].sizex+x] = (byte)value;
		break;

	default:
		Sys_Error("vc_SetTile: invalid layer value (");
	}
}

void vc_ScaleSprite()
{
	int		x, y, source_width, source_length, dest_width, dest_length;
	int		ptr;

	x				=ResolveOperand();
	y				=ResolveOperand();
	source_width	=ResolveOperand();
	source_length	=ResolveOperand();
	dest_width		=ResolveOperand();
	dest_length		=ResolveOperand();
	ptr				=ResolveOperand();

	LucentOn && translucency_table
	? CopySpriteZoomLucentClip(x,y, source_width,source_length, dest_width,dest_length, (byte *)ptr)
	: CopySpriteZoomClip(x,y, source_width,source_length, dest_width,dest_length, (byte *)ptr);
}

void vc_EntityMove()
{
	int		ent;
	string	movescript;

	ent=ResolveOperand();
	movescript=ResolveString();

	if (ent<0 || ent>=entities)
	{
		Log(va("vc_EntityMove: no such entity %d", ent));
		return;
	}

	entity[ent].moving=0;
	entity[ent].speedct=0;

	entity[ent].delayct=0;
	entity[ent].mode=0;
	entity[ent].data1=0;

	strncpy(movescriptbuf+256*ent, movescript.c_str(), 255);

	entity[ent].scriptofs=movescriptbuf+256*ent;
	entity[ent].movecode=4;
}

void vc_HLine()
{
	int		x, y, xe, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	xe		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? HLineLucentClip(x,y,xe, color)
	: HLineClip(x,y,xe, color);
}

void vc_VLine()
{
	int		x, y, ye, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	ye		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? VLineLucentClip(x,y,ye, color)
	: VLineClip(x,y,ye, color);
}

void vc_Line()
{
	int		x, y, xe, ye, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	xe		=ResolveOperand();
	ye		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? LineLucentClip(x,y, xe,ye, color)
	: LineClip(x,y, xe,ye, color);
}

void vc_Circle()
{
	int		x, y, radius, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	radius	=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? CircleLucentClip(x,y, radius, color)
	: CircleClip(x,y, radius, color);
}

void vc_CircleFill()
{
	int		x, y, radius, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	radius	=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? CircleFillLucentClip(x,y, radius, color)
	: CircleFillClip(x,y, radius, color);
}

void vc_Rect()
{
	int		x, y, xe, ye, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	xe		=ResolveOperand();
	ye		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? RectLucentClip(x,y, xe,ye, color)
	: RectClip(x,y, xe,ye, color);
}

void vc_RectFill()
{
	int		x, y, xe, ye, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	xe		=ResolveOperand();
	ye		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? RectFillLucentClip(x,y, xe,ye, color)
	: RectFillClip(x,y, xe,ye, color);
}

void vc_strlen()
{
	string	text;

	text	=ResolveString();

	vcreturn=text.length();
}

void vc_strcmp()
{
	string	a, b;

	a	=ResolveString();
	b	=ResolveString();

	if (a<b)
		vcreturn=-1;
	else if (a>b)
		vcreturn=+1;
	else
		vcreturn=0;
}

void vc_FontWidth()
{
	int		n;

	n	=ResolveOperand();	// slot

	vcreturn=Font_GetWidth(n);
}

void vc_FontHeight()
{
	int		n;

	n	=ResolveOperand();

	vcreturn=Font_GetLength(n);
}

void vc_SetPixel()
{
	int		x, y, color;

	x		=ResolveOperand();
	y		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? SetPixelLucentClip(x,y, color)
	: SetPixelClip(x,y, color);
}

void vc_GetPixel()
{
	int		x, y;

	x	=ResolveOperand();
	y	=ResolveOperand();

	vcreturn=GetPixelClip(x,y);
}

void vc_EntityOnScreen()
{
	int		find, n;

	find=ResolveOperand();
	for (n=0; n<cc; n++)
	{
		if (entidx[n]==find)
		{
			vcreturn=1;
			return;
		}
	}

	vcreturn=0;
}

void vc_GetTile()
{
	int x, y, lay;

	x	=ResolveOperand();
	y	=ResolveOperand();
	lay	=ResolveOperand();

	vcreturn=0;

// ensure all arguments are valid
	if (x<0 || y<0)
		return;
	if (lay==6 || lay==7)
	{
		if (x>=layer[0].sizex || y>=layer[0].sizey)
			return;
	}
	else
	{
		if ((lay>=0 && lay<6)
		&& (x>=layer[lay].sizex || y>=layer[lay].sizey))
		{
			return;
		}
	}

	switch (lay)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		vcreturn=(int)layers[lay][y*layer[lay].sizex+x];
		break;
	case 6:
		vcreturn=(int)obstruct[y*layer[0].sizex+x];
		break;
	case 7:
		vcreturn=(int)zone[y*layer[0].sizex+x];
		break;

	default:
		Sys_Error("vc_GetTile: invalid layer value");
	}
}

void vc_SetResolution()
{
	int		xres, yres;

	xres	= ResolveOperand();
	yres	= ResolveOperand();

	vcreturn = GFX_SetMode(xres, yres);
}

void vc_SetRString()
{
// TODO: some validity checks would be nice
	rstring=ResolveString();
}

void vc_SetClipRect()
{
	int		bogus;

	clip.x		= ResolveOperand();
	clip.y		= ResolveOperand();
	clip.xend	= ResolveOperand();
	clip.yend	= ResolveOperand();

	bogus	= 0;

// ensure arguments stay valid
	if (clip.x < 0)
		clip.x = 0,		bogus++;
	else if (clip.x >= screen_width)
		clip.x = screen_width - 1,	bogus++;
	if (clip.y < 0)
		clip.y = 0,		bogus++;
	else if (clip.y >= screen_length)
		clip.y = screen_length - 1,	bogus++;

	if (clip.xend<0)
		clip.xend=0,		bogus++;
	else if (clip.xend>=screen_width)
		clip.xend=screen_width-1,	bogus++;
	if (clip.yend<0)
		clip.yend=0,		bogus++;
	else if (clip.yend>=screen_length)
		clip.yend=screen_length-1,	bogus++;

	if (bogus)
		Log(va("vc_SetClipRect: %d bogus args", bogus));
}

void vc_SetRenderDest()
{
	screen_width = ResolveOperand();
	screen_length = ResolveOperand();
	clip.x = 0;
	clip.y = 0;
	clip.xend = screen_width - 1;
	clip.yend = screen_length - 1;
	screen = (byte *) ResolveOperand();
}

void vc_RestoreRenderSettings()
{
	screen_width = true_screen_width;
	screen_length = true_screen_length;
	clip.x = 0;
	clip.y = 0;
	clip.xend = screen_width - 1;
	clip.yend = screen_length - 1;
	screen = vscreen;
}

void vc_PartyMove()
{
	player=0;

	entity[playernum].moving=0;
	entity[playernum].speedct=0;
	entity[playernum].delayct=0;
	entity[playernum].mode=0;
	entity[playernum].data1=0;

	vcpush(cameratracking);
	vcpush(tracker);

	if (1==cameratracking)
	{
		cameratracking=2;
		tracker=playernum;
	}

	//ResolveString((char *) (int) movescriptbuf + (int) (playernum*256));
	string movescript=ResolveString();
	strncpy(movescriptbuf+256*playernum, movescript.c_str(), 255);
	movescriptbuf[256*playernum+255]='\0';

	entity[playernum].scriptofs=movescriptbuf+256*playernum;
	entity[playernum].movecode=4;

	while (entity[playernum].movecode)
	{
		while (timer_count)
		{
			ProcessEntities();
			timer_count--;
		}
		Render();
		ShowPage();
	}

	tracker=(byte)vcpop();
	cameratracking=(byte)vcpop();

	player=&entity[playernum];
	timer_count=0;
}

void vc_WrapBlit()
{
	int		offsetx, offsety, width, length;
	int		ptr;

	offsetx	= ResolveOperand();
	offsety	= ResolveOperand();
	width	= ResolveOperand();
	length	= ResolveOperand();
	ptr		= ResolveOperand();

	LucentOn && translucency_table
	? LFB_BlitWrapLucent_8(offsetx, offsety, width, length, (byte *) ptr, 0)
	: LFB_BlitWrap_8(offsetx, offsety, width, length, (byte *) ptr, 0);
}


void vc_TWrapBlit()
{
	int		offsetx, offsety, width, length;
	int		ptr;

	offsetx	= ResolveOperand();
	offsety	= ResolveOperand();
	width	= ResolveOperand();
	length	= ResolveOperand();
	ptr		= ResolveOperand();

	LucentOn && translucency_table
	? LFB_BlitWrapLucent_8(offsetx, offsety, width, length, (byte *) ptr, 1)
	: LFB_BlitWrap_8(offsetx, offsety, width, length, (byte *) ptr, 1);
}

void vc_SetMousePos()
{
	int		x, y;

	x	= ResolveOperand();
	y	= ResolveOperand();

	Mouse_SetPosition(x, y);
}

void vc_HookRetrace()
{
	int		script;

	script = 0;
	switch (GrabC())
	{
	case 1:
		script = ResolveOperand();
		break;
	case 2:
		script = USERFUNC_MARKER + GrabD();
		break;
	}
	hookretrace = script;
}

void vc_HookTimer()
{
	int		script;

	script = 0;
	switch (GrabC())
	{
	case 1: script = ResolveOperand(); break;
	case 2: script = USERFUNC_MARKER + GrabD(); break;
	}
	hooktimer = script;
}

void vc_HookKey()
{
	int		key, script;

	key = ResolveOperand();
	if (key < 0) key = 0;
	if (key > 127) key = 127;
	key = scantokey[key];

	script = 0;
	switch (GrabC())
	{
	case 1:
		script = ResolveOperand();
		break;
	case 2:
		script = USERFUNC_MARKER + GrabD();
		break;
	}
	bindarray[key] = script;
}

void vc_PlayMusic()
{
	string	filename;

	filename	= ResolveString();

	PlayMusic(filename.c_str());
}

static int morph_step(int S, int D, int mix, int light)
{
	return (mix*(S - D) + (100*D))/100*light/64;
}

static unsigned char pmorph_palette[3*256];
void vc_PaletteMorph()
{
	int		rgb[3];
	int		n, percent, intensity;

	CheckHookTimer();

	rgb[0] = ResolveOperand();	// red
	rgb[1] = ResolveOperand();	// green
	rgb[2] = ResolveOperand();	// blue

	percent		= 100 - ResolveOperand();
	intensity	= ResolveOperand();

// keep components valid
	for (n = 0; n < 3; n += 1)
	{
		if (rgb[n] < 0)
			rgb[n] = 0;
		else if (rgb[n] > 63)
			rgb[n] = 63;
	}

// pull the colors
	for (n = 0; n < 3*256; n += 1)
	{
		pmorph_palette[n] = (unsigned char) morph_step(game_palette[n], rgb[n % 3], percent, intensity);
	}

// enforce new palette
	GFX_SetPalette(pmorph_palette);
}

string EnforceNoDirectories(string s)
{
	int		n;

// find rightmost slash
	n = s.length() - 1;
	while (n && s[n] != '/' && s[n] != '\\')
		n -= 1;
// step past it
	if (s[n] == '/' || s[n] == '\\')
		n += 1;

	return s.right(s.length() - n);
}

void vc_OpenFile()
{
	string	filename;
	VFILE*	vf;

	filename	= ResolveString();
	filename	= EnforceNoDirectories(filename);

	vf = vopen(filename.c_str());
	vcreturn = (quad)vf;

	Log(va(" --> VC opened file %s, ptr 0x%08X", filename.c_str(), (quad) vf));
}

void vc_CloseFile()
{
	VFILE*	vf;

	vf	=(VFILE *)ResolveOperand();
	vclose(vf);

	Log(va(" --> VC closed file at ptr 0x%08X", (quad)vf));
}

void vc_QuickRead()
{
	/*
	int		temp;

	Log("vc_QuickRead disabled.");

	ResolveString();	// filename

	temp=GrabC();			// code
	GrabW();				// offset
	if (op_SARRAY==temp)
		ResolveOperand();	// offset
	ResolveOperand();		// seek line
	*/

	string	filename, ret;
	int		offset, seekline;

	filename	=ResolveString();
	filename	=EnforceNoDirectories(filename);

// which string are we reading into?
	char code=GrabC();
	if (code==op_STRING)
	{
		offset=GrabW();
	}
	if (code==op_SARRAY)
	{
		offset=GrabW();
		offset+=ResolveOperand();
	}

// which line are we reading from the file?
	seekline = ResolveOperand();
	if (seekline < 1)
		seekline = 1;

// open the file
	VFILE* f=vopen(filename.c_str());
	if (!f)
	{
		Sys_Error("vc_QuickRead: could not open %s", filename.c_str());
	}

// seek to the line of interest
	char temp[256]={0};
	for (int n=0; n<seekline; n++)
	{
		vgets(temp, 255, f);
	}
// suppress trailing CR/LF
	char *p=temp;
	while (*p)
	{
		if ('\n' == *p || '\r' == *p)
			*p = '\0';
		p++;
	}

// assign to vc string
	if (offset>=0 && offset<stralloc)
	{
		vc_strings[offset]=temp;
	}

	vclose(f);
}

void vc_AddFollower()
{
	Log("vc_AddFollower disabled.");

	ResolveOperand();	// entity

	/*
	int n;

	n=ResolveOperand();
	if (n<0 || n>=entities)
	{
		Sys_Error("vc_AddFollower: Not a valid entity index. (%d)", n);
	}

	follower[(int)numfollowers++]=n;
//	ResetFollowers();
	*/
}

void vc_FlatPoly()
{
	Log("vc_FlatPoly disabled.");

	ResolveOperand();	// a
	ResolveOperand();	// b
	ResolveOperand();	// c
	ResolveOperand();	// d
	ResolveOperand();	// e
	ResolveOperand();	// f
	ResolveOperand();	// g

	/*
  int a, b, c, d, e, f, g;

  a=ResolveOperand();
  b=ResolveOperand();
  c=ResolveOperand();
  d=ResolveOperand();
  e=ResolveOperand();
  f=ResolveOperand();
  g=ResolveOperand();
  FlatPoly(a,b,c,d,e,f,g);
	*/
}

void vc_TMapPoly()
{
	Log("vc_TMapPoly disabled.");

	ResolveOperand();	// a
	ResolveOperand();	// b
	ResolveOperand();	// c
	ResolveOperand();	// d
	ResolveOperand();	// e
	ResolveOperand();	// f
	ResolveOperand();	// g
	ResolveOperand();	// h
	ResolveOperand();	// i
	ResolveOperand();	// j
	ResolveOperand();	// k
	ResolveOperand();	// l
	ResolveOperand();	// m
	ResolveOperand();	// n
	ResolveOperand();	// o

	/*
  int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o;

  a=ResolveOperand();
  b=ResolveOperand();
  c=ResolveOperand();
  d=ResolveOperand();
  e=ResolveOperand();
  f=ResolveOperand();
  g=ResolveOperand();
  h=ResolveOperand();
  i=ResolveOperand();
  j=ResolveOperand();
  k=ResolveOperand();
  l=ResolveOperand();
  m=ResolveOperand();
  n=ResolveOperand();
  o=ResolveOperand();
  TMapPoly(a,b,c,d,e,f,g,h,i,j,k,l,m,n,(char *) o);
	*/
}

void vc_CacheSound()
{
	string	filename;

	filename	=ResolveString();

	vcreturn=CacheSound(filename.c_str());
}

void vc_PlaySound()
{
	int		slot, volume, pan;

	slot	=ResolveOperand();
	volume	=ResolveOperand();
	pan		=ResolveOperand();

	PlaySound(slot, volume, pan);
}

void vc_RotScale()
{
	int		x, y, width, length, angle, scale;
	int		ptr;

	x		=ResolveOperand();
	y		=ResolveOperand();
	width	=ResolveOperand();
	length	=ResolveOperand();
	angle	=ResolveOperand();
	scale	=ResolveOperand();
	ptr		=ResolveOperand();

	RotScale(x, y, width, length, angle*3.14159/180.0, scale/1000.0, (byte *)ptr);
}

void vc_MapLine()
{
	Log("vc_MapLine disabled.");

	ResolveOperand();	// x
	ResolveOperand();	// y
	ResolveOperand();	// screen_length
	ResolveOperand();	// l

	/*
  int x, y, screen_length, l;
  int xtc, ytc, xofs, yofs;

  x=ResolveOperand();
  y=ResolveOperand();
  screen_length=ResolveOperand();
  l=ResolveOperand();

  xtc=x >> 4;
  ytc=y >> 4;
  xofs= -(x&15);
  yofs=  (y&15);

  MapLine(xofs, screen_length, yofs, (word *) (layers[l]+((ytc*layer[l].sizex)+xtc)));
	*/
}

void vc_TMapLine()
{
	Log("vc_TMapLine disabled.");

	ResolveOperand();	// x
	ResolveOperand();	// y
	ResolveOperand();	// screen_length
	ResolveOperand();	// l

	/*
  int x, y, screen_length, l;
  int xtc, ytc, xofs, yofs;

  x=ResolveOperand();
  y=ResolveOperand();
  screen_length=ResolveOperand();
  l=ResolveOperand();

  xtc=x >> 4;
  ytc=y >> 4;
  xofs= -(x&15);
  yofs=  (y&15);

  TMapLine(xofs, screen_length, yofs, (word *) (layers[l]+((ytc*layer[l].sizex)+xtc)));
	*/
}

void vc_val()
{
	string	s;

	s	=ResolveString();

	vcreturn=atoi(s.c_str());
}

void vc_TScaleSprite()
{
	int		x, y, source_width, source_length, dest_width, dest_length;
	int		ptr;

	x				=ResolveOperand();
	y				=ResolveOperand();
	source_width	=ResolveOperand();
	source_length	=ResolveOperand();
	dest_width		=ResolveOperand();
	dest_length		=ResolveOperand();
	ptr				=ResolveOperand();

	LucentOn && translucency_table
	? TCopySpriteZoomLucentClip(x,y, source_width,source_length, dest_width,dest_length, (byte *)ptr)
	: TCopySpriteZoomClip(x,y, source_width,source_length, dest_width,dest_length, (byte *)ptr);
}

void vc_GrabRegion()
{
	int		x, y, xe, ye, bogus;
	int		ptr;

	x	=ResolveOperand();
	y	=ResolveOperand();
	xe	=ResolveOperand();
	ye	=ResolveOperand();
	ptr	=ResolveOperand();

	bogus=0;

// ensure arguments stay valid
	if (x<0)
		x=0,	bogus++;
	else if (x>=screen_width)
		x=screen_width-1,	bogus++;
	if (y<0)
		y=0,	bogus++;
	else if (y>=screen_length)
		y=screen_length-1,	bogus++;

	if (xe<0)
		xe=0,	bogus++;
	else if (xe>=screen_width)
		xe=screen_width-1,bogus++;
	if (ye<0)
		ye=0,	bogus++;
	else if (ye>=screen_length)
		ye=screen_length-1,bogus++;

	if (bogus)
		Log(va("vc_GrabRegion: %d bogus args", bogus));

// swap?
	if (xe<x) {
		int t=x;
		x=xe;
		xe=t;
	}
	if (ye<y) {
		int t=ye;
		y=ye;
		ye=t;
	}

	xe=xe-x+1;
	ye=ye-y+1;

	unsigned char* source=screen+(y*screen_width)+x;
	while (ye)
	{
		memcpy((void *)ptr, source, xe);

		ptr+=xe;
		source+=screen_width;
		ye--;
	}
}

void vc_Log()
{
	string	message;

	message	=ResolveString();

	Log(message.c_str());
}

void vc_fseekline()
{
	int		line;
	VFILE*	vf;

	line	=ResolveOperand();
	vf		=(VFILE *)ResolveOperand();

	vseek(vf, 0, SEEK_SET);

// 0 & 1 will yield first line
	char temp[256+1];
	do
	{
		vgets(temp, 256, vf);
	} while (--line > 0);
}

void vc_fseekpos()
{
	int		pos;
	VFILE*	vf;

	pos	=ResolveOperand();
	vf	=(VFILE *)ResolveOperand();

	vseek(vf, pos, 0);
}

void vc_fread()
{
	char*	buffer;
	int		len;
	VFILE*	vf;

	buffer	=(char *)ResolveOperand();
	len		=ResolveOperand();
	vf		=(VFILE *)ResolveOperand();

	vread(buffer, len, vf);
}

void vc_fgetbyte()
{
	VFILE*	vf	=0;
	byte	b	=0;

	vf	=(VFILE *)ResolveOperand();
	vread(&b, 1, vf);

	vcreturn=b;
}

void vc_fgetword()
{
	VFILE*	vf	=0;
	word	w	=0;

	vf	=(VFILE *)ResolveOperand();
	vread(&w, 2, vf);

	vcreturn=w;
}

void vc_fgetquad()
{
	VFILE*	vf	=0;
	quad	q	=0;

	vf	=(VFILE *)ResolveOperand();
	vread(&q, 4, vf);

	vcreturn=q;
}

void vc_fgetline()
{
	char	temp[256 +1];
	char*	p;
	int		code, offset;
	VFILE*	vf;

// which global vc string do we read into?
	code=GrabC();
	offset=GrabW();
	if (op_SARRAY == code)
	{
		offset+=ResolveOperand();
	}

// file pointer; blegh
	vf	=(VFILE *)ResolveOperand();

// read line into temp buffer
	vgets(temp, 256, vf);
	temp[256]='\0';

// suppress trailing CR/LF
	p=temp;
	while (*p)
	{
		if ('\n' == *p || '\r' == *p)
			*p = '\0';
		p++;
	}

// assign to vc string
	if (offset>=0 && offset<stralloc)
	{
		vc_strings[offset]=temp;
	}
}

void vc_fgettoken()
{
	char	temp[256];
	int		code, offset;
	VFILE*	vf;

// which global vc string do we read into?
	code=GrabC();
	offset=GrabW();
	if (code == op_SARRAY)
	{
		offset+=ResolveOperand();
	}

// file pointer; blegh
	vf	=(VFILE *)ResolveOperand();

// read into temp buffer
	vscanf(vf, "%s", temp);

// assign to vc string
	if (offset>=0 && offset<stralloc)
	{
		vc_strings[offset]=temp;
	}
}

void vc_fwritestring()
{
	FILE*	f;
	string	temp;

	temp	=ResolveString();
	f		=(FILE *)ResolveOperand();

	fprintf(f, "%s\n", temp.c_str());
}

void vc_fwrite()
{
	char*	buffer;
	int		length;
	FILE*	f;

	buffer	=(char *)ResolveOperand();
	length	=ResolveOperand();
	f		=(FILE *)ResolveOperand();

	fwrite(buffer, 1, length, f);
}

void vc_frename()
{
	string	a, b;

	a=ResolveString();
	b=ResolveString();
	a=EnforceNoDirectories(a);
	b=EnforceNoDirectories(b);

	rename(a.c_str(), b.c_str());

	Log(va(" --> VC renamed %s to %s.", a.c_str(), b.c_str()));
}

void vc_fdelete()
{
	string	filename;

	filename=ResolveString();
	filename=EnforceNoDirectories(filename);

	remove(filename.c_str());

	Log(va(" --> VC deleted %s.", filename.c_str()));
}

void vc_fwopen()
{
	string	filename;

	filename	=ResolveString();
	filename	=EnforceNoDirectories(filename);

	vcreturn=(int)fopen(filename.c_str(),"wb");

	Log(va(" --> VC opened %s for writing, ptr %u.", filename.c_str(), vcreturn));
}

void vc_fwclose()
{
	FILE*	f;

	f=(FILE *)ResolveOperand();
	fclose(f);

	Log(va(" --> VC close file opened for writing, ptr %u.", (int)f));
}

void vc_memcpy()
{
	char	*source, *dest;
	int		length;

	dest	=(char *)ResolveOperand();
	source	=(char *)ResolveOperand();
	length	=ResolveOperand();

	memcpy(dest, source, length);
}

void vc_memset()
{
	char*	dest;
	int		color, length;

	dest	=(char *)ResolveOperand();
	color	=ResolveOperand();
	length	=ResolveOperand();

	V_memset(dest, color, length);
}

// <aen, may 5>
// + modified to use new silhouette vdriver routines
// + added checks for ClipOn
void vc_Silhouette()
{
	int		x, y, width, length, color;
	int		ptr;

	x		=ResolveOperand();
	y		=ResolveOperand();
	width	=ResolveOperand();
	length	=ResolveOperand();
	ptr		=ResolveOperand();
	color	=ResolveOperand();

	LucentOn && translucency_table
	? SilhouetteLucentClip(x,y, width,length, color, (byte *)ptr)
	: SilhouetteClip(x,y, width,length, color, (byte *)ptr);
}

void vc_Mosaic()
{
	Log("vc_Mosaic disabled.");

	ResolveOperand();	// a
	ResolveOperand();	// b
	ResolveOperand();	// c
	ResolveOperand();	// d
	ResolveOperand();	// e
	ResolveOperand();	// f
	ResolveOperand();	// g

	/*
  int a,b,c,d,e,f,g;

  a=ResolveOperand();
  b=ResolveOperand();
  c=ResolveOperand();
  d=ResolveOperand();
  e=ResolveOperand();
  f=ResolveOperand();
  g=ResolveOperand();
  Mosaic(a,b,(byte *) c,d,e,f,g);
	*/
}

void vc_WriteVars()
{
	FILE*	f;

	f	=(FILE *)ResolveOperand();

	fwrite(globalint, 1, 4*maxint, f);

	for (int n=0; n<stralloc; n++)
	{
		int z=vc_strings[n].length();

		fwrite(&z, 1, 4, f);
		fwrite(vc_strings[n].c_str(), 1, z, f);
		fputc(0, f);
	}
	//fwrite(stringbuf, 256, stralloc, f);
}

void vc_ReadVars()
{
	FILE*	f;

	f	=(FILE *)ResolveOperand();

	fread(globalint, 1, 4*maxint, f);

	for (int n=0; n<stralloc; n++)
	{
		int z;
		fread(&z, 1, 4, f);
		char* temp=new char[z+1];
		if (!temp)
		{
			Sys_Error("vc_Readars: memory exhausted on %d bytes.", z);
		}
		fread(temp, 1, z, f);
		temp[z]='\0';
		vc_strings[n]=temp;

		delete[] temp;
		temp=0;
	}
}

void vc_Asc()
{
	vcreturn=ResolveString()[0];
}

void vc_NumForScript()
{
	vcreturn=GrabD();
}

void vc_Filesize()
{
	string	filename;
	VFILE*	vf;

	filename	=ResolveString();

	vf=vopen(filename.c_str());
	vcreturn=filesize(vf);
	vclose(vf);
}

void vc_FTell()
{
	VFILE*	vf;

	vf	=(VFILE *)ResolveOperand();

	vcreturn=vtell(vf);
}

void vc_CheckCorrupt()
{
	Log("vc_CheckCorrupt disabled.");

	/*
	Log("checking for corruption...");
	CheckCorruption();
	*/
}

void vc_ChangeCHR()
{
	int		who;
	string	chrname;

	who=ResolveOperand();
	chrname=ResolveString();

	ChangeCHR(who, chrname.c_str());
}