#ifndef CDROM_H
#define CDROM_H

#ifdef __WATCOMC__
#pragma pack (1)
#include <i86.h>
#endif

#define EJECT_TRAY     0
#define RESET          2
#define CLOSE_TRAY     5
#define DATA_TRACK    64
#define LOCK           1
#define UNLOCK         0
#define BUSY         512
#define MEDIA_CHANGE   9
#define TRACK_MASK   208

//#pragma pack (1);

struct DPMI_PTR
{
  unsigned short int segment;
  unsigned short int selector;
};

struct CD_d
{
  unsigned short Drives;
  unsigned char  First_drive;
  unsigned short Current_track;
  unsigned long  Track_position;
  unsigned char  Track_type;
  unsigned char  Low_audio;
  unsigned char  High_audio;
  unsigned char  Disk_length_min;
  unsigned char  Disk_length_sec;
  unsigned char  Disk_length_frames;
  unsigned long  Endofdisk;
  unsigned char  UPC[7];
  unsigned char  DiskID[6];
  unsigned long  Status;
  unsigned short Error;
};

struct CD_d CD_Cdrom_data;

typedef struct CD_Playinfo
{
  unsigned char  Control;
  unsigned char  Adr;
  unsigned char  Track;
  unsigned char  Index;
  unsigned char  Min;
  unsigned char  Sec;
  unsigned char  Frame;
  unsigned char  Zero;
  unsigned char  Amin;
  unsigned char  Asec;
  unsigned char  Aframe;
} CD_Playinfo;

struct CD_Volumeinfo
{
  unsigned char  Mode;
  unsigned char  Input0;
  unsigned char  Volume0;
  unsigned char  Input1;
  unsigned char  Volume1;
  unsigned char  Input2;
  unsigned char  Volume2;
  unsigned char  Input3;
  unsigned char  Volume3;
} CD_Volumeinfo;

// ------------------------------------------------------------------------
//                      CDROM Prototypes
// ------------------------------------------------------------------------
unsigned long CD_Head_position (void);
void CD_Get_volume (void);
void CD_Set_volume (void);
short CD_GetUPC (void);
void CD_Get_Audio_info (void);
void CD_Set_Track (short Tracknum);
void CD_Track_length (short Tracknum, unsigned char * min, unsigned char * sec, unsigned char * frame);
void CD_Status (void);
void CD_Seek (unsigned long Location);
void CD_Play_Audio (unsigned long Begin, unsigned long End);
void CD_Stop_Audio (void);
void CD_Resume_Audio (void);
void CD_Cmd (unsigned char mode);
void CD_Getpos (struct CD_Playinfo * info);
short CD_Installed (void);
short CD_Done_play (void);
short CD_Mediach (void);
void CD_Lock (unsigned char Doormode);
void CD_Init (void);
void CD_DeInit (void);
// ------------------------------------------------------------------------
//                          DPMI Support Functions..
// ------------------------------------------------------------------------
void DPMI_AllocDOSMem (short int paras, struct DPMI_PTR *p);
void DPMI_FreeDOSMem (struct DPMI_PTR *p);
// ------------------------------------------------------------------------


#endif
