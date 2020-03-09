#include "cdrom.h"
#include "verge.h"

// prototypes
void CD_CMD (unsigned char Mode);

#pragma pack (1)

static struct rminfo {
  unsigned long EDI;
  unsigned long ESI;
  unsigned long EBP;
  unsigned long reserved_by_system;
  unsigned long EBX;
  unsigned long EDX;
  unsigned long ECX;
  unsigned long EAX;
  unsigned short flags;
  unsigned short ES,DS,FS,GS,IP,CS,SP,SS;
} RMI;

static struct DPMI_PTR CD_Device_req   = {0,0};
static struct DPMI_PTR CD_Device_extra = {0,0};
static union  REGS  regs;
static struct SREGS sregs;

static void PrepareRegisters (void)
{
  V_memset(&RMI,0,sizeof(RMI));
  V_memset(&sregs,0,sizeof(sregs));
  V_memset(&regs,0,sizeof(regs));
}

static void RMIRQ (char irq)
{
  V_memset (&regs, 0, sizeof (regs));
  regs.w.ax =  0x0300;
  regs.h.bl =  irq;
  sregs.es =   FP_SEG(&RMI);
  regs.x.edi = FP_OFF(&RMI);
  int386x(0x31, &regs, &regs, &sregs);
}

void DPMI_AllocDOSMem (short int paras, struct DPMI_PTR *p)
{
  PrepareRegisters();
  regs.w.ax=0x0100;
  regs.w.bx=paras;
  int386x( 0x31, &regs, &regs, &sregs);
  p->segment=regs.w.ax;
  p->selector=regs.w.dx;
}

void DPMI_FreeDOSMem (struct DPMI_PTR *p)
{
  V_memset(&sregs,0,sizeof(sregs));
  regs.w.ax=0x0101;
  regs.w.dx=p->selector;
  int386x( 0x31, &regs, &regs, &sregs);
}

void CD_Device_Request (void)
{
  PrepareRegisters();
  RMI.EAX = 0x01510;
  RMI.ECX = CD_Cdrom_data.First_drive;
  RMI.EDI = 0;
  RMI.ES  = CD_Device_req.segment;
  RMIRQ (0x02F);
}

void Red_book (unsigned long Value, unsigned char * min, unsigned char * sec, unsigned char * frame)
{
  *frame  = (Value & 0x000000FF);
  *sec    = (Value & 0x0000FF00) >> 8;
  *min    = (Value & 0x00FF0000) >> 16;
}

unsigned long HSG (unsigned long Value)
{
  unsigned char min, sec, frame;

  Red_book (Value, &min, &sec, &frame);
  Value    = (unsigned long) min * 4500;
  Value   += (short)         sec * 75;
  Value   += frame - 150;
  return (Value);
}

short CD_Cdrom_installed (void)
{
  DPMI_AllocDOSMem (4, &CD_Device_req);
  DPMI_AllocDOSMem (2, &CD_Device_extra);

  PrepareRegisters();
  regs.x.eax = 0x01500;
  int386 (0x02F, &regs, &regs);

  if (regs.x.ebx ==0) return(0);
  CD_Cdrom_data.Drives      = (short)regs.x.ebx;
  CD_Cdrom_data.First_drive = (short)regs.x.ecx;
  CD_Get_Audio_info ();
  return (1);
}

void CD_Get_Audio_info (void)
{
typedef struct IOCTLI {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} IOCTLI;
typedef struct Track_data {
  unsigned char  Mode;
  unsigned char  Lowest;
  unsigned char  Highest;
  unsigned long  Address;
} Track_data;

  static struct IOCTLI     * IOCTLI_Pointers;
  static struct Track_data * Track_data_Pointers;

  IOCTLI_Pointers     = (struct IOCTLI *)     (CD_Device_req.segment*16);
  Track_data_Pointers = (struct Track_data *) (CD_Device_extra.segment*16);

  V_memset (IOCTLI_Pointers, 0, sizeof (struct IOCTLI));
  V_memset (Track_data_Pointers, 0, sizeof (struct Track_data));

  IOCTLI_Pointers->Length   = sizeof (struct IOCTLI);
  IOCTLI_Pointers->Comcode  = 3;
  IOCTLI_Pointers->Address  = CD_Device_extra.segment<<16;
  IOCTLI_Pointers->Bytes    = sizeof (struct Track_data);
  Track_data_Pointers->Mode = 0x0A;
  CD_Device_Request();

  V_memcpy(&CD_Cdrom_data.DiskID, &Track_data_Pointers->Lowest, 6);
  CD_Cdrom_data.Low_audio  = Track_data_Pointers->Lowest;
  CD_Cdrom_data.High_audio = Track_data_Pointers->Highest;
  Red_book (Track_data_Pointers->Address, &CD_Cdrom_data.Disk_length_min, &CD_Cdrom_data.Disk_length_sec, &CD_Cdrom_data.Disk_length_frames);
  CD_Cdrom_data.Endofdisk = HSG ( Track_data_Pointers->Address);
  CD_Cdrom_data.Error     = IOCTLI_Pointers->Status;
}

unsigned long CD_Get_track_length (short Tracknum)
{
  unsigned long Start, Finish;
  unsigned short CT;

  CT = CD_Cdrom_data.Current_track;
  CD_Set_Track (Tracknum);
  Start = CD_Cdrom_data.Track_position;
  if (Tracknum < CD_Cdrom_data.High_audio)
  {
   CD_Set_Track ( Tracknum + 1);
   Finish = CD_Cdrom_data.Track_position;
  }
  else Finish = CD_Cdrom_data.Endofdisk;

  CD_Set_Track (CT);

  Finish -= Start;
  return ( Finish);
}

void CD_Track_length (short Tracknum, unsigned char * min, unsigned char * sec, unsigned char * frame)
{
  unsigned long Value;

  Value    = CD_Get_track_length (Tracknum);
  Value   += 150;
  * frame  = Value % 75;
  Value   -= * frame;
  Value   /= 75;
  * sec    = Value % 60;
  Value   -= * sec;
  Value   /= 60;
  * min    = Value;
}

short CD_Done_play (void)
{
  CD_CMD (CLOSE_TRAY);
  return ((CD_Cdrom_data.Error & BUSY) == 0);
}

unsigned long CD_Head_Position (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
  unsigned char  Unused2[4];
} Tray_request;
typedef struct Head_data {
  unsigned char  Mode;
  unsigned char  Adr_mode;
  unsigned long  Address;
} Head_data;

  static struct Tray_request     * Tray_request_Pointers;
  static struct Head_data        * Head_data_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  Head_data_Pointers    = (struct Head_data *)     (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (Head_data_Pointers, 0, sizeof (struct Head_data));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 6;
  Head_data_Pointers->Mode        = 0x01;
  Head_data_Pointers->Adr_mode    = 0x00;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
  return (Head_data_Pointers->Address);
}

void CD_Set_Track (short Tracknum)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;
typedef struct Track_data {
  unsigned char  Mode;
  unsigned char  Track;
  unsigned long  Address;
  unsigned char  Control;
} Head_data;

  static struct Tray_request      * Tray_request_Pointers;
  static struct Track_data        * Track_data_Pointers;

  Tray_request_Pointers  = (struct Tray_request *)     (CD_Device_req.segment*16);
  Track_data_Pointers    = (struct Track_data *)       (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (Track_data_Pointers, 0, sizeof (struct Track_data));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 7;

  Track_data_Pointers->Mode       = 0x0B;
  Track_data_Pointers->Track      = Tracknum;

  CD_Device_Request();

  CD_Cdrom_data.Error          = Tray_request_Pointers->Status;
  CD_Cdrom_data.Track_position = HSG (Track_data_Pointers->Address);
  CD_Cdrom_data.Current_track  = Tracknum;
  CD_Cdrom_data.Track_type     = Track_data_Pointers->Control & TRACK_MASK;
}

void CD_Status (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;
typedef struct CD_data {
  unsigned char  Mode;
  unsigned long  Status;
} CD_data;

  static struct Tray_request      * Tray_request_Pointers;
  static struct CD_data           * CD_data_Pointers;

  Tray_request_Pointers  = (struct Tray_request *)     (CD_Device_req.segment*16);
  CD_data_Pointers       = (struct CD_data *)          (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (CD_data_Pointers, 0, sizeof (struct CD_data));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 5;
  CD_data_Pointers->Mode          = 0x06;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
  CD_Cdrom_data.Status     = CD_data_Pointers->Status;
}

void CD_Seek (unsigned long Location)
{
//  unsigned char min, sec, frame;
typedef struct Play_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Addressmode;
  unsigned long  Transfer;
  unsigned short Sector;
  unsigned long  Seekpos;
} Play_request;

  static struct Play_request      * Play_request_Pointers;

  Play_request_Pointers  = (struct Play_request *)     (CD_Device_req.segment*16);

  V_memset (Play_request_Pointers, 0, sizeof (struct Play_request));

  Play_request_Pointers->Length   = sizeof (struct Play_request);
  Play_request_Pointers->Comcode  = 131;
  Play_request_Pointers->Seekpos  = Location;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Play_request_Pointers->Status;
}

void CD_Stop_Audio (void)
{
typedef struct Stop_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
} Stop_request;

  static struct Stop_request      * Stop_request_Pointers;

  Stop_request_Pointers  = (struct Stop_request *)     (CD_Device_req.segment*16);

  V_memset (Stop_request_Pointers, 0, sizeof (struct Stop_request));

  Stop_request_Pointers->Length   = sizeof (struct Stop_request);
  Stop_request_Pointers->Comcode  = 133;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Stop_request_Pointers->Status;
}

void CD_Resume_Audio (void)
{
typedef struct Stop_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
} Stop_request;

  static struct Stop_request      * Stop_request_Pointers;

  Stop_request_Pointers  = (struct Stop_request *)     (CD_Device_req.segment*16);

  V_memset (Stop_request_Pointers, 0, sizeof (struct Stop_request));

  Stop_request_Pointers->Length   = sizeof (struct Stop_request);
  Stop_request_Pointers->Comcode  = 136;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Stop_request_Pointers->Status;
}

void CD_Play_Audio (unsigned long Begin, unsigned long End)
{
typedef struct Play_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Addressmode;
  unsigned long  Start;
  unsigned long  Playlength;
} Play_request;

  static struct Play_request      * Play_request_Pointers;

  Play_request_Pointers  = (struct Play_request *)     (CD_Device_req.segment*16);

  V_memset (Play_request_Pointers, 0, sizeof (struct Play_request));

  Play_request_Pointers->Length     = sizeof (struct Play_request);
  Play_request_Pointers->Comcode    = 132;
  Play_request_Pointers->Start      = Begin;
  Play_request_Pointers->Playlength = End-Begin;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Play_request_Pointers->Status;
}

void CD_CMD (unsigned char Mode)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
  unsigned char  Unused2[4];
} Tray_request;
typedef struct CD_Mode {
  unsigned char  Mode;
} CD_Mode;

  static struct Tray_request     * Tray_request_Pointers;
  static struct CD_Mode          * CD_Mode_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  CD_Mode_Pointers =      (struct CD_Mode *)       (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (CD_Mode_Pointers, 0, sizeof (struct CD_Mode));

  CD_Mode_Pointers->Mode = Mode;
  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 12;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 1;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
}

void CD_Lock (unsigned char Doormode)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned char  Unused2[4];
} Tray_request;

typedef struct CD_Data {
  unsigned char  Mode;
  unsigned char  Media;
} CD_Data;

  static struct Tray_request     * Tray_request_Pointers;
  static struct CD_Data          * CD_Data_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  CD_Data_Pointers =      (struct CD_Data *)       (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (CD_Data_Pointers, 0, sizeof (struct CD_Data));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 12;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 2;
  CD_Data_Pointers->Mode          = 1;
  CD_Data_Pointers->Media         = Doormode;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
}

short CD_Mediach (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;

typedef struct CD_Data {
  unsigned char  Mode;
  unsigned char  Media;
} CD_Data;

  static struct Tray_request     * Tray_request_Pointers;
  static struct CD_Data          * CD_Data_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  CD_Data_Pointers =      (struct CD_Data *)       (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (CD_Data_Pointers, 0, sizeof (struct CD_Data));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 2;
  CD_Data_Pointers->Mode          = 0x09;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
  return (CD_Data_Pointers->Media);
}

void CD_Get_volume (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;

  static struct Tray_request     * Tray_request_Pointers;
  static struct CD_Volumeinfo    * CD_Volume_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  CD_Volume_Pointers    = (struct CD_Volumeinfo *) (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 9;
  CD_Volume_Pointers->Mode        = 0x04;

  CD_Device_Request();
  V_memcpy(&CD_Volumeinfo, CD_Volume_Pointers, sizeof(struct CD_Volumeinfo));
  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
}

void CD_Set_volume (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;


  static struct Tray_request     * Tray_request_Pointers;
  static struct CD_Volumeinfo    * CD_Volume_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  CD_Volume_Pointers    = (struct CD_Volumeinfo *) (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memcpy (CD_Volume_Pointers, &CD_Volumeinfo, sizeof(struct CD_Volumeinfo));
  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 12;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 9;
  CD_Volume_Pointers->Mode        = 0x03;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
}


short CD_GetUPC (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;
typedef struct UPC_data {
  unsigned char Mode;
  unsigned char Adr;
  unsigned char UPC[17];
  unsigned char Zero;
  unsigned char Aframe;
} UPC_data;

  static struct Tray_request     * Tray_request_Pointers;
  static struct UPC_data         * UPC_data_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  UPC_data_Pointers     = (struct UPC_data *)      (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (UPC_data_Pointers, 0, sizeof (struct UPC_data));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 11;
  UPC_data_Pointers->Mode         = 0x0E;
  UPC_data_Pointers->Adr          = 0x02;

  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
   if (UPC_data_Pointers->Adr == 0)
     V_memset(&UPC_data_Pointers->UPC,0,7);
   V_memcpy(&CD_Cdrom_data.UPC[0], &UPC_data_Pointers->UPC[0],7);
   return 1;
}

void CD_Getpos (void)
{
typedef struct Tray_request {
  unsigned char  Length;
  unsigned char  Subunit;
  unsigned char  Comcode;
  unsigned short Status;
  unsigned char  Unused[8];
  unsigned char  Media;
  unsigned long  Address;
  unsigned short Bytes;
  unsigned short Sector;
  unsigned long  VolID;
} Tray_request;

  static struct Tray_request     * Tray_request_Pointers;
  static struct CD_Playinfo      * CD_Playinfo_Pointers;

  Tray_request_Pointers = (struct Tray_request *)  (CD_Device_req.segment*16);
  CD_Playinfo_Pointers  = (struct CD_Playinfo *)   (CD_Device_extra.segment*16);

  V_memset (Tray_request_Pointers, 0, sizeof (struct Tray_request));
  V_memset (CD_Playinfo_Pointers, 0, sizeof (struct CD_Playinfo));

  Tray_request_Pointers->Length   = sizeof (struct Tray_request);
  Tray_request_Pointers->Comcode  = 3;
  Tray_request_Pointers->Address  = CD_Device_extra.segment<<16;
  Tray_request_Pointers->Bytes    = 6;
  CD_Playinfo_Pointers->Control   = 12;
  CD_Device_Request();

  CD_Cdrom_data.Error      = Tray_request_Pointers->Status;
}

void CD_DeInit (void)
{
  DPMI_FreeDOSMem (&CD_Device_req);
  DPMI_FreeDOSMem (&CD_Device_req);
}

void CD_Init (void)
{
  if (nocdaudio) return;
  if (!CD_Cdrom_installed())
      Message("MSCDEX not found", 500);
}

void CD_Stop()
{
  if (nocdaudio) return;
  CD_Stop_Audio();
  CD_Lock(UNLOCK);
}

void CD_Close_Door()
{
  if (nocdaudio) return;
  CD_Cmd(5);
}

void CD_Open_Door()
{
  if (nocdaudio) return;
  CD_Cmd(0);
}

void CD_Play(int track)
{
  if (nocdaudio) return;
  CD_Set_Track(track);
  CD_Lock(LOCK);
  CD_Seek(CD_Cdrom_data.Track_position);
  delay(400);
  CD_Play_Audio(CD_Cdrom_data.Track_position, CD_Cdrom_data.Endofdisk);
  timer_count=0;
}
