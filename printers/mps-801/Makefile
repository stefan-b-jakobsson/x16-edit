SRC_FILES=$(wildcard *.asm) $(wildcard *.inc)

X16EDITPD-MPS801.DRV: $(SRC_FILES)
	cl65 -o $@ -t cx16 -C printer.cfg mps801.asm
