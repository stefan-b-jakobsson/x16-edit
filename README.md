# x16-edit

Text editor for Commander X16

X16 Edit is a simple text editor for the Commander X16 platform inspired by GNU Nano but lacking some of its more advanced functions such as soft line breaks.

Text entered by the user is stored in banked RAM.

The program is designed to handle large text files with reasonable performance.


# Compiling

The program is written in 65c02 assembly for the ca65 compiler.

Compile with the following command:

cl65 -v -t cx16 -C cx16-asm.cfg -u __EXEHDR__ -o x16edit.prg main.asm


# Running

The program is tested with X16 emulator version R38.

The emulator may be downloaded from

https://www.commanderx16.com/

Run the program with the following command

./x16emu -sdcard sdcard.img -prg x16edit.prg -run


Loading and saving files in X16 Edit require that the emulator is started with an attached sdcard. Use the switch -sdcard.


# X16 Community

You may read more about the Commander X16 Platform on the website

https://www.commanderx16.com/
