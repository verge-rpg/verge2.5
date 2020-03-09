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
// ³                 Entity and Player Movement module                   ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <zero, 5.6.98>
// + corrected oversight in movement script management by sticking a hack in
//   MoveScript().  Bug caused Fx commands to not work sometimes.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "verge.h"

// ================================= Data ====================================

chrlist_r chrlist[100];
byte nmchr,playernum;
entity_r *player=0;
entity_r entity[256];
byte entities=0;

chrdata chr[100];

byte numchrs=0;
byte entidx[256];
byte cc;
byte movesuccess;

// ================================= Code ====================================

int ObstructionAt(int tx, int ty)
{
	if (tx<0 || tx>=layer[0].sizex || ty<0 || ty>=layer[0].sizey)
		return 1;

	return obstruct[(ty*layer[0].sizex)+tx];
}

int Zone(int tx, int ty)
{
	if (tx<0 || tx>=layer[0].sizex || ty<0 || ty>=layer[0].sizey)
		return 0;

	return zone[(ty*layer[0].sizex)+tx];
}

void LoadCHR(char *fname, chrdata *c)
{
	VFILE*	f;
	char	b;
	int		n;
	char*	ptr;

	V_strlwr(fname);

	f	=vopen(fname);
	if (!f)
	{
		Sys_Error("Could not open CHR file %s.", fname);
	}

	strncpy(c->fname, fname, 59);
	c->fname[59]='\0';

	vread(&b, 1, f);
	if (b != 2)
		Sys_Error("CHR %s incorrect CHR format version.", fname);
	vread(&c->fxsize, 2, f);
	vread(&c->fysize, 2, f);
	vread(&c->hx, 2, f);
	vread(&c->hy, 2, f);

	// Modified by Pyro
	vread(&c->hw, 2, f);
	vread(&c->hh, 2, f);

	//vread(strbuf, 4, f);	// skip the hotspot size.

	vread(&c->totalframes, 2, f);

	vread(&n, 4, f);
	ptr=(char *) valloc(n, "LoadCHR:ptr", OID_TEMP);
	vread(ptr, n, f);
	c->imagedata	=new byte [c->fxsize*c->fysize*c->totalframes]; //(byte *) valloc(c->fxsize * c->fysize * c->totalframes, "LoadCHR:c->imagedata", OID_IMAGE);

	n=ReadCompressedLayer1(c->imagedata, c->fxsize * c->fysize * c->totalframes, ptr);
	if (n)
	{
		Sys_Error("LoadCHR: %s: bogus compressed image data", fname);
	}
	vfree(ptr);

	if (hicolor)
	{
		unsigned short* _16 = new unsigned short [c->fxsize*c->fysize*c->totalframes];
		for (n=0; n<c->fxsize*c->fysize*c->totalframes; n++)
		{
			if (!c->imagedata[n])
				_16[n] = (16 == hicolor) ? 0xF81F : 0x7C1F;
			else
				_16[n] = LoToHi(c->imagedata[n]);
		}
		delete[] c->imagedata;
		c->imagedata = (byte *)_16;
	}

	vread(&c->lidle, 4, f);
	vread(&c->ridle, 4, f);
	vread(&c->uidle, 4, f);
	vread(&c->didle, 4, f);

	for (b=0; b<4; b++)
	{
		switch (b)
		{
			case 0: ptr=c->lanim; break;
			case 1: ptr=c->ranim; break;
			case 2: ptr=c->uanim; break;
			case 3: ptr=c->danim; break;
		}
		vread(&n, 4, f);
		if (n>100)
			Sys_Error("Animation strand too long. %d", n);
		vread(ptr, n, f);
	}
	vclose(f);
}

int CacheCHR(char *fname)
{
	if (numchrs >= 100)
		Sys_Error("CacheCHR: too many chrs loaded: %d", numchrs);

	LoadCHR(fname, &chr[numchrs]);
	return numchrs++;
}

void FreeCHRList()
{
	int		n	=0;

	for (n=0; n<numchrs; n++)
	{
		//vfree(chr[i].imagedata);
		delete[] chr[n].imagedata;
	}
	V_memset(chr, 0, sizeof(chr));
}

void LoadCHRList()
{
	int		n;

	for (n=0; n<nmchr; n++)
	{
		if (V_strlen(chrlist[n].t))
			CacheCHR(chrlist[n].t);
	}
}

void DrawEntity(int i)
{
	int		a, b, dx, dy;
	unsigned char*	ptr;

	dx=entity[i].x-xwin;
	dy=entity[i].y-ywin;
	a=entity[i].chrindex;
	if (a<0 || a>=numchrs) return;
	b=entity[i].specframe ? entity[i].specframe : entity[i].frame;

	if (b<0 || b >= chr[a].totalframes)
		Sys_Error("DrawEntity: invalid frame request: %d", b);

	ptr=(unsigned char *)(chr[a].imagedata+(b*chr[a].fxsize*chr[a].fysize)*(hicolor?2:1));

	LFB_Blit(
		dx-chr[a].hx,
		dy-chr[a].hy,
		chr[a].fxsize,
		chr[a].fysize,
		ptr, 1, 0);
}

static int cmpent(const void* a, const void* b)
{
	return entity[*(byte*)a].y - entity[*(byte*)b].y;
}

void RenderEntities()
{
	int		n;

	qsort(entidx, cc, 1, cmpent);
	for (n=0; n<cc; n++)
		DrawEntity(entidx[n]);
}

int GetArg(entity_r *p)
{
	int		n;
	static char	token[10];

	while (*p->animofs==' ')
		p->animofs++;

	n=0;
	while (*p->animofs>=48 && *p->animofs<=57)
	{
		token[n++]=*p->animofs++;
	}
	token[n]=0;

	return V_atoi(token);
}

void GetNextCommand(entity_r* p)
{
	while (' ' == *p->animofs)
		p->animofs++;

	switch (*p->animofs++)
	{
		case 'f':
		case 'F':
			p->frame=(byte)GetArg(p);
			break;
		case 'w':
		case 'W':
			p->delayct=(byte)GetArg(p);
			break;

		case 0:
			if (p->moving)
			{
				switch (p->facing)
				{
					case 0: p->animofs=chr[p->chrindex].danim; break;
					case 1: p->animofs=chr[p->chrindex].uanim; break;
					case 2: p->animofs=chr[p->chrindex].lanim; break;
					case 3: p->animofs=chr[p->chrindex].ranim; break;
				}
			}
			else
			{
				switch (p->facing)
				{
					case 0: p->animofs=0; p->frame=(byte)chr[p->chrindex].didle; break;
					case 1: p->animofs=0; p->frame=(byte)chr[p->chrindex].uidle; break;
					case 2: p->animofs=0; p->frame=(byte)chr[p->chrindex].lidle; break;
					case 3: p->animofs=0; p->frame=(byte)chr[p->chrindex].ridle; break;
				}
			}
			p->delayct=0;
			break;
	}
}

void AnimateEntity(entity_r *p)
{
	if (!p->animofs && p->moving)
	{
		switch (p->facing)
		{
			case 0: p->animofs=chr[p->chrindex].danim; break;
			case 1: p->animofs=chr[p->chrindex].uanim; break;
			case 2: p->animofs=chr[p->chrindex].lanim; break;
			case 3: p->animofs=chr[p->chrindex].ranim; break;
		}
		p->delayct=0;
	}
	if (!p->animofs && !p->moving)
	{
		switch (p->facing)
		{
			case 0: p->animofs=0; p->frame=(byte)chr[p->chrindex].didle; break;
			case 1: p->animofs=0; p->frame=(byte)chr[p->chrindex].uidle; break;
			case 2: p->animofs=0; p->frame=(byte)chr[p->chrindex].lidle; break;
			case 3: p->animofs=0; p->frame=(byte)chr[p->chrindex].ridle; break;
		}
		p->delayct=0;
	}
	if (p->delayct)
	{
		p->delayct--;
	}
	else
		GetNextCommand(p);
}

int EntityAt(int ex, int ey)
{
	int		n;

	for (n=0; n<cc; n++)
	{
		if (&entity[entidx[n]]==player)
			continue;
		if (ex==entity[entidx[n]].tx && ey==entity[entidx[n]].ty)
			return entidx[n]+1;
	}
	return 0;
}

int EntityObsAt(int ex, int ey)
{
	int		n;

	for (n=0; n<cc; n++)
	{
		if (&entity[entidx[n]]==player)
			continue;
		if (ex==entity[entidx[n]].tx && ey==entity[entidx[n]].ty
		&&	entity[entidx[n]].obsmode2)
			return entidx[n]+1;
	}
	return 0;
}

int AEntityObsAt(int ex, int ey)
{
	int		n;

	for (n=0; n<cc; n++)
	{
		if (ex==entity[entidx[n]].tx && ey==entity[entidx[n]].ty
		&&	entity[entidx[n]].obsmode2)
			return entidx[n]+1;
	}
	return 0;
}

void SiftEntities()
{
	int n, dx, dy;

	V_memset(entidx, 0, 256); cc=0;
	for (n=0; n<entities; n++)
	{
		dx=entity[n].x-xwin+16;
		dy=entity[n].y-ywin+16;

		if (dx<0 || dx>screen_width+chr[entity[n].chrindex].fxsize)
			continue;
		if (dy<0 || dy>screen_length+chr[entity[n].chrindex].fysize)
			continue;

		entidx[cc++]=(byte)n;
	}
}

void MoveRight(int i)
{
	int		tx, ty;

	tx=entity[i].tx+1;
	ty=entity[i].ty;

	if (entity[i].obsmode1
	&& (ObstructionAt(tx,ty) || AEntityObsAt(tx,ty)))
	{
		movesuccess=0;
		return;
	}
	if (entity[i].facing != 3)
	{
		entity[i].delayct=0;
		entity[i].animofs=0;
	}

	entity[i].x++;
	entity[i].facing=3;
	entity[i].moving=4;
	entity[i].ctr=15;
	entity[i].tx++;
	movesuccess=1;
	entity[i].reset=0;
}

void MoveLeft(int i)
{
	int		tx, ty;

	tx=entity[i].tx-1;
	ty=entity[i].ty;

	if (entity[i].obsmode1
	&& (ObstructionAt(tx,ty) || AEntityObsAt(tx,ty)))
	{
		movesuccess=0;
		return;
	}
	if (entity[i].facing != 2)
	{
		entity[i].delayct=0;
		entity[i].animofs=0;
	}

	entity[i].x--;
	entity[i].facing=2;
	entity[i].moving=3;
	entity[i].ctr=15;
	entity[i].tx--;
	movesuccess=1;
	entity[i].reset=0;
}

void MoveUp(int i)
{
	int		tx, ty;

	tx=entity[i].tx;
	ty=entity[i].ty-1;

	if (entity[i].obsmode1
	&& (ObstructionAt(tx,ty) || AEntityObsAt(tx,ty)))
	{
		movesuccess=0;
		return;
	}
	if (entity[i].facing != 1)
	{
		entity[i].delayct=0;
		entity[i].animofs=0;
	}

	entity[i].y--;
	entity[i].facing=1;
	entity[i].moving=2;
	entity[i].ctr=15;
	entity[i].ty--;
	movesuccess=1;
	entity[i].reset=0;
}

void MoveDown(int i)
{
	int		tx, ty;

	tx=entity[i].tx;
	ty=entity[i].ty+1;

	if (entity[i].obsmode1
	&& (ObstructionAt(tx,ty) || AEntityObsAt(tx,ty)))
	{
		movesuccess=0;
		return;
	}
	if (entity[i].facing!=0)
	{
		entity[i].delayct=0;
		entity[i].animofs=0;
	}

	entity[i].y++;
	entity[i].facing=0;
	entity[i].moving=1;
	entity[i].ctr=15;
	entity[i].ty++;
	movesuccess=1;
	entity[i].reset=0;
}

void Wander1(int i)
{
	if (!entity[i].data1)
	{
		entity[i].data2=(word)rnd(0,3);
		entity[i].data1=(word)(entity[i].step+1);
	}
	if (entity[i].data1==1)
	{
		entity[i].delayctr++;
		if (entity[i].delayctr>=entity[i].delay)
			entity[i].data1=0;
		return;
	}
	if (entity[i].data1>1)
	{
		switch(entity[i].data2)
		{
			case 0: MoveUp(i); break;
			case 1: MoveDown(i); break;
			case 2: MoveLeft(i); break;
			case 3: MoveRight(i); break;
		}
		entity[i].data1--;
		if (entity[i].data1==1)
		{
			entity[i].delayctr=0;
			entity[i].animofs=0;
			entity[i].delayct=0;
		}
	}
}

void Wander2(int i)
{
	if (!entity[i].data1)
	{
		entity[i].data3=(word)rnd(0,3);
		entity[i].data1=(word)(entity[i].step+1);
	}
	if (entity[i].data1==1)
	{
		entity[i].delayctr++;
		if (entity[i].delayctr>=entity[i].delay)
			entity[i].data1=0;
		return;
	}
	if (entity[i].data1>1)
	{
		switch(entity[i].data3)
		{
			case 0:
				if (Zone(entity[i].tx,entity[i].ty-1)==entity[i].data2)
					MoveUp(i);
				break;
			case 1:
				if (Zone(entity[i].tx,entity[i].ty+1)==entity[i].data2)
					MoveDown(i);
				break;
			case 2:
				if (Zone(entity[i].tx-1,entity[i].ty)==entity[i].data2)
					MoveLeft(i);
				break;
			case 3:
				if (Zone(entity[i].tx+1,entity[i].ty)==entity[i].data2)
					MoveRight(i);
				break;
		}
		entity[i].data1--;
		if (entity[i].data1==1)
			entity[i].delayctr=0;
	}
}

void Wander3(int i)
{
	if (!entity[i].data1)
	{
		entity[i].data2=(word)rnd(0,3);
		entity[i].data1=(word)(entity[i].step+1);
	}
	if (entity[i].data1==1)
	{
		entity[i].delayctr++;
		if (entity[i].delayctr>=entity[i].delay)
			entity[i].data1=0;
		return;
	}
	if (entity[i].data1>1)
	{
		switch(entity[i].data2)
		{
			case 0:
				if (entity[i].ty>entity[i].data3)
					MoveUp(i);
				break;
			case 1:
				if (entity[i].ty<entity[i].data6)
					MoveDown(i);
				break;
			case 2:
				if (entity[i].tx>entity[i].data2)
					MoveLeft(i);
				break;
			case 3:
				if (entity[i].tx<entity[i].data5)
					MoveRight(i);
				break;
		}
		entity[i].data1--;
		if (entity[i].data1==1)
			entity[i].delayct=0;
	}
}

void Whitespace(int i)
{
	while (' ' == *entity[i].scriptofs)
		entity[i].scriptofs++;
}

void GetArgMS(int i)
{
	int		j;
	char	token[10];

	j=0;
	Whitespace(i);
	while (*entity[i].scriptofs>=48 && *entity[i].scriptofs<=57)
	{
		token[j++]=*entity[i].scriptofs++;
	}
	token[j]='\0';
	entity[i].data1=(word)V_atoi(token);
}

void GetNextCommandMS(int i)
{
	Whitespace(i);
	switch (*entity[i].scriptofs++)
	{
		case 'u':
		case 'U':
			entity[i].mode=1;
			GetArgMS(i);
			break;
		case 'd':
		case 'D':
			entity[i].mode=2;
			GetArgMS(i);
			break;
		case 'l':
		case 'L':
			entity[i].mode=3;
			GetArgMS(i);
			break;
		case 'r':
		case 'R':
			entity[i].mode=4;
			GetArgMS(i);
			break;
		case 's':
		case 'S':
			entity[i].mode=5;
			GetArgMS(i);
			break;
		case 'w':
		case 'W':
			entity[i].mode=6;
			GetArgMS(i);
			entity[i].animofs=0;
			entity[i].delayct=0;
			break;

		case 0:
			switch (entity[i].facing)
			{
				case 0:
					entity[i].animofs=0;
					entity[i].frame=(byte)chr[entity[i].chrindex].didle;
					break;
				case 1:
					entity[i].animofs=0;
					entity[i].frame=(byte)chr[entity[i].chrindex].uidle;
					break;
				case 2:
					entity[i].animofs=0;
					entity[i].frame=(byte)chr[entity[i].chrindex].lidle;
					break;
				case 3:
					entity[i].animofs=0;
					entity[i].frame=(byte)chr[entity[i].chrindex].ridle;
					break;
			}
			entity[i].movecode=0;
			entity[i].mode=7;
			entity[i].data1=0;
			entity[i].scriptofs=0;
			entity[i].delayct=0;
			break;

		case 'c':
		case 'C':
			entity[i].mode=8;
			GetArgMS(i);
			break;
		case 'b':
		case 'B':
			entity[i].mode=9;
			break;
		case 'x':
		case 'X':
			entity[i].mode=10;
			GetArgMS(i);
			break;
		case 'y':
		case 'Y':
			entity[i].mode=11;
			GetArgMS(i);
			break;
		case 'f':
		case 'F':
			entity[i].mode=12;
			GetArgMS(i);
			break;
		case 'z':
		case 'Z':
			entity[i].mode=13;
			GetArgMS(i);
			break;

		default:
			Sys_Error("Invalid entity movement script.");
	}
}

void MoveScript(int i)
{
	if (!entity[i].scriptofs)
		entity[i].scriptofs	=ms+(int)msbuf[entity[i].movescript];

	if (!entity[i].mode)
		GetNextCommandMS(i);

	switch (entity[i].mode)
	{
		case 1:
			MoveUp(i);
			if (movesuccess)
				entity[i].data1--;
			break;
		case 2:
			MoveDown(i);
			if (movesuccess)
				entity[i].data1--;
			break;
		case 3:
			MoveLeft(i);
			if (movesuccess)
				entity[i].data1--;
			break;
		case 4:
			MoveRight(i);
			if (movesuccess)
				entity[i].data1--;
			break;
		case 5:
			entity[i].speed=(byte)entity[i].data1;
			entity[i].data1=0;
			break;
		case 6:
			entity[i].data1--;
			break;
		case 7:
			return;
		case 8:
			lastent=i;
			ExecuteEvent(entity[i].data1);
			entity[i].data1=0;
			break;
		case 9:
			entity[i].scriptofs	=ms+(int)msbuf[entity[i].movescript];
			entity[i].data1=0;
			break;
		case 10:
			if (entity[i].tx<entity[i].data1)
				MoveRight(i);
			if (entity[i].tx>entity[i].data1)
				MoveLeft(i);
			if (entity[i].tx==entity[i].data1)
				entity[i].data1=0;
			break;
		case 11:
			if (entity[i].ty<entity[i].data1)
				MoveDown(i);
			if (entity[i].ty>entity[i].data1)
				MoveUp(i);
			if (entity[i].ty==entity[i].data1)
				entity[i].data1=0;
			break;
		case 12:
			entity[i].facing=(byte)entity[i].data1;
			entity[i].data1=0;
			//--- zero 5.6.99
			switch (entity[i].facing)
			{
				case 0:
					entity[i].frame=(byte)chr[entity[i].chrindex].didle;
					break;
				case 1:
					entity[i].frame=(byte)chr[entity[i].chrindex].uidle;
					break;
				case 2:
					entity[i].frame=(byte)chr[entity[i].chrindex].lidle;
					break;
				case 3:
					entity[i].frame=(byte)chr[entity[i].chrindex].ridle;
					break;
			}
			//---
			break;
		case 13:
			entity[i].specframe=(byte)entity[i].data1;
			entity[i].data1=0;
			break;
	}

	if (!entity[i].data1)
		entity[i].mode=0;
}

void TestActive(int i)
{
	int		dx, dy;

	dx	=abs(entity[i].x - player->x);
	dy	=abs(entity[i].y - player->y);
	if ((dx<=16 && dy<=3) || (dx<=3 && dy<=16))
	{
		if (!entity[i].expand4 && !invc)
		{
			entity[i].expand4=1;
			ExecuteEvent(entity[i].actscript);
		}
	}
	else
		entity[i].expand4=0;
}

void ProcessEntity1(int i)
{
	entity[i].speedct=0;
	if (entity[i].actm)
		TestActive(i);

	if (!entity[i].moving)
	{
		switch (entity[i].movecode)
		{
			case 0:
				return;
			case 1:
				Wander1(i);
				break;
			case 2:
				Wander2(i);
				break;
			case 3:
				Wander3(i);
				break;
			case 4:
				MoveScript(i);
				break;

			default:
				Sys_Error("ProcessEntity1: unknown entity movement pattern: %d", entity[i].movecode);
		}
		if (!entity[i].reset)
			return;
	}

	if (entity[i].reset)
	{
		entity[i].animofs=0;
		entity[i].delayct=0;
		entity[i].reset=0;
	}

	if (entity[i].moving)
	{
		if (entity[i].moving==1)
		{
			entity[i].y++;
			entity[i].ctr--;
			AnimateEntity(&entity[i]);
		}
		if (entity[i].moving==2)
		{
			entity[i].y--;
			entity[i].ctr--;
			AnimateEntity(&entity[i]);
		}
		if (entity[i].moving==4)
		{
			entity[i].x++;
			entity[i].ctr--;
			AnimateEntity(&entity[i]);
		}
		if (entity[i].moving==3)
		{
			entity[i].x--;
			entity[i].ctr--;
			AnimateEntity(&entity[i]);
		}

		if (!entity[i].ctr)
		{
			entity[i].reset=1;
			entity[i].moving=0;
		}
	}
}

void ProcessEntity(int i)
{
	int j;

	if (player==&entity[i])
		return;
	if (entity[i].speed<4)
	{
		switch (entity[i].speed)
		{
			case 1:
				if (entity[i].speedct<3)
				{
					entity[i].speedct++;
					return;
				}
			case 2:
				if (entity[i].speedct<2)
				{
					entity[i].speedct++;
					return;
				}
			case 3:
				if (entity[i].speedct<1)
				{
					entity[i].speedct++;
					return;
				}
		}
	}
	if (entity[i].speed<5)
	{
		ProcessEntity1(i);
		entity[i].speedct=0;
	}
	switch (entity[i].speed)
	{
		case 5:
			for (j=0; j<2; j++)
			{
				ProcessEntity1(i);
			}
			return;
		case 6:
			for (j=0; j<3; j++)
			{
				ProcessEntity1(i);
			}
			return;
		case 7:
			for (j=0; j<4; j++)
			{
				ProcessEntity1(i);
			}
			return;
	}
}

void ProcessEntities()
{
	int		n;

	 SiftEntities();
	 for (n=0; n<entities; n++)
	 	ProcessEntity(n);
}

int FindCHR(char* fname)
{
	int		n	=0;

	V_strlwr(fname);
	for (n=0; n<numchrs; n++)
	{
		if (!V_strcmp(chr->fname, fname))
			return n;
	}

	return -1;
}

int AllocateEntity(int x1, int y1, char *fname)
{
	int		n	=0;

	n=FindCHR(fname);
	entity[entities].chrindex=(byte)((n > -1)?n:CacheCHR(fname));

	entity[entities].x=x1*16;
	entity[entities].y=y1*16;
	entity[entities].tx=(word)x1;
	entity[entities].ty=(word)y1;
	entity[entities].speed=4;
	entity[entities].obsmode1=1;
	entity[entities].obsmode2=1;

	return entities++;
}

void ChangeCHR(int who, char* chrname)
{
	int		n	=0;

	if (who<0 || who>=256)
		Sys_Error("ChangeCHR: no such entity: %d", who);

	n=FindCHR(chrname);
	entity[who].chrindex=(byte)((n > -1)?n:CacheCHR(chrname));
}