SRC_FILES=$(wildcard *.asm) $(wildcard *.inc)

X16EDITPD-GENERIC-ESCP.DRV: $(SRC_FILES)
	cl65 -o $@ -t cx16 -C printer.cfg escp.asm
