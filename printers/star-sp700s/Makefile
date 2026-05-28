SRC_FILES=$(wildcard *.asm) $(wildcard *.inc)

X16EDITPD-SP700s.DRV: $(SRC_FILES)
	cl65 -o $@ -t cx16 -C printer.cfg -l sp700s.lst sp700s.asm
clean:
	rm *.DRV *.o *.lst

