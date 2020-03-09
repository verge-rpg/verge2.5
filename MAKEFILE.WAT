
CC = wpp386
CFLAGS = /u_WIN32 /oneatx /zp4 /j /d0 /wx

OBJ = linked.obj rawmem.obj rawstr.obj mouse.obj controls.obj keyboard.obj &
      modeinit.obj startup.obj timer.obj vdriver.obj &
      console.obj engine.obj entity.obj a_memory.obj render.obj &
      image.obj message.obj verge.obj vfile.obj vc.obj font.obj &
      sound.obj fli.obj dpmi.obj vbe2.obj

.CC: watcom

.CC.OBJ:
        $(CC) $(CFLAGS) $[*
        wlib verge.lib +-$^&

.CC.EXE:
        wcl386 /bt=dos /l=dos4g /k60000 $(CFLAGS) $[* verge.lib mikwat.lib

.ASM: watcom

.ASM.OBJ:
        wasm $[*
        wlib verge.lib +-$^&

verge.exe: $(OBJ)
