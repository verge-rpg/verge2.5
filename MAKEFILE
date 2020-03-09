CC=gcc
CFLAGS= -s -Wall -Werror -m486 -O1 -ffast-math # GCC
#CFLAGS= -Wall -mpentium -O6 -ffast-math -g # PGCC

OBJ = conlib.o console.o djgpp/controls.o engine.o entity.o font.o image.o djgpp/keyboard.o message.o djgpp/modeinit.o render.o djgpp/startup.o djgpp/timer.o verge.o vc.o djgpp/vdriver.o vfile.o fli.o djgpp/cdrom.o sound.o mikmod.a djgpp/bcd.o a_memory.o

verge: $(OBJ)

