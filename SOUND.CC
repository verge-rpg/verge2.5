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
// ³                        Sound system module                          ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#define SOUND_H
#include "verge.h"

// ================================= Data ====================================

UNIMOD *mf;
SAMPLE *sfx[100];
int nsfx=0;
char playing[60+1];

// ================================= Code ====================================

void PlaySound(int, int, int);

void tickhandler()
{
  MP_HandleTick();
  MD_SetBPM(mp_bpm);
}

void InitMusicSystem()
{
  V_memset(playing, 0, 60);
  ML_RegisterLoader(&load_mod);
  ML_RegisterLoader(&load_s3m);
  ML_RegisterLoader(&load_uni);
  ML_RegisterLoader(&load_xm);

  MD_RegisterDriver(&drv_nos);
//  MD_RegisterDriver(&drv_ss);
  MD_RegisterDriver(&drv_sb);
  MD_RegisterDriver(&drv_gus);

  MD_RegisterPlayer(tickhandler);
  if (!MD_Init())
  {
    printf("Driver error: %s.\n",myerr);
    exit(-1);
  }
}

#pragma off (unreferenced);
void PlayMusic(char *fname)
{
  V_strlwr(fname);
  if (!V_strcmp(fname,playing)) return;
  if (!MP_Ready())
  {
    MD_PlayStop();
    ML_Free(mf);
  }
  //V_memcpy(playing, fname, V_strlen(fname));
  V_strncpy(playing, fname, 60);
  playing[60]='\0';
  mf=ML_LoadFN(fname);
  if (!mf) Sys_Error(myerr);
  MP_Init(mf);
  md_numchn=(UBYTE)(mf->numchn+2);
  mp_loop=1; mp_volume=100;
  MD_PlayStart();
  PlaySound(0, 0, 0);
  PlaySound(0, 0, 0);
}

void StopMusic()
{
  if (MP_Ready()) return;
  MD_PlayStop();
  ML_Free(mf);
}

int CacheSound(char *fname)
{
  sfx[nsfx]=MW_LoadWavFN(fname);
  if (!sfx[nsfx])
    Sys_Error("WAV [%s] load error: %s", fname, myerr);
  return nsfx++;
}

void FreeAllSounds()
{
  int i;

  for (i=0; i<nsfx; i++)
      MW_FreeWav(sfx[i]);
  nsfx=0;
}

void PlaySound(int index, int vol, int pan)
{
  static int switcher=1;
  int chanl;

  if (index >= nsfx) return;
  switcher ^= 1;
  chanl=md_numchn-1-switcher;
  MD_VoiceSetVolume((UBYTE)chanl, (UBYTE)vol);
  MD_VoiceSetPanning((UBYTE)chanl, pan);
  MD_VoiceSetFrequency((UBYTE)chanl, sfx[index]->c2spd);
  MD_VoicePlay((UBYTE)chanl, sfx[index]->handle, 0, sfx[index]->length,
               0, 0, sfx[index]->flags);
}
