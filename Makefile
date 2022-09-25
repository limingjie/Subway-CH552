TARGET = subway

FREQ_SYS = 16000000

C_FILES = \
	main.c \
	bitbang.c \
	../ch554_sdcc/include/debug.c

ASM_FILES = \
	bitbang_asm.asm

pre-flash:

include Makefile.include
