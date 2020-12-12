# x16-edit

Text editor for Commander X16

X16 Edit is a simple text editor for the Commander X16 platform inspired by GNU Nano.

Text entered by the user is stored in banked RAM (512 KB, expandable to 2 MB).

The program is designed to handle large text files with good performance. It
is not intended to be a word processor, and will support only basic text
editor functions.


# Building

The program is written in 65c02 assembly for the ca65 compiler.

Currently, there are two build targets.

The first target (the RAM version) is to be loaded as a normal program from the filesystem. It has a BASIC header, and is started with the RUN command.

The second target is a 16 KB image to be stored in one of the ROM banks (the ROM version).


# Running the RAM version

The program is tested with X16 emulator version R38.

The emulator may be downloaded from

https://www.commanderx16.com/

Run the RAM version with the following command

x16emu -sdcard sdcard.img -prg X16EDIT.PRG -run

Loading and saving files in X16 Edit require that the emulator is started with an attached sdcard.


# Running the ROM version

There are a few more steps to set up and try the ROM version.

The ROM version build script generates a 16KB image to be stored in one of the ROM banks. To get a working system, you need to append this image to the ROM image distributed with
the emulator ("rom.bin"). This may be done with the following command (Linux/MacOS):

cat rom.bin x16edit-rom.bin > customrom.bin

On starting the emulator you need to specify the custom rom image as follows:

x16emu -rom customrom.bin -sdcard sdcard.img

To start the ROM version of the editor you need to type in a small startup routine in the built-in monitor, for example:

.A1000 LDA $9F60
.A1003 PHA
.A1004 LDA #$07
.A1006 STA $9F60
.A1009 LDX #$01
.A100B LDY #$FF
.A100D JSR $C000
.A1010 PLA
.A1011 STA $9F60
.A1014 RTS

To run this from BASIC, symply type SYS $1000.

A short explanation of the startup program. $9F60 is the ROM select. The original value is first stored on the stack, and then we switch to ROM bank 7 where X16Edit is stored. The ROM version of the editor requires that the first and last RAM bank used by the program are specified (X=first bank and Y=last bank). Thereafter it calls the subroutine at $C000, which is the start of ROM and the entry point of the editor. When exiting from the editor the initial ROM bank is read from the stack stored in the ROM select.


# X16 Community

You may read more about the Commander X16 Platform on the website

https://www.commanderx16.com/
