SRC_FILES=$(wildcard *.asm) $(wildcard *.inc)

X16EDITPD-SP700.DRV: $(SRC_FILES)
	cl65 -o $@ -t cx16 -C printer.cfg sp700.asm
