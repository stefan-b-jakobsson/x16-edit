SRC_FILES=$(wildcard *.asm) $(wildcard *.inc) $(wildcard *.ps)

X16EDITPD-POSTSCRIPT.DRV: $(SRC_FILES)
	cl65 -o $@ -t cx16 -C printer.cfg -m postscript.map postscript.asm
