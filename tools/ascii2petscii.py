import sys

f = open(sys.argv[1])
f2 = open(sys.argv[1] + ".pet", "w")

c = f.read(1)
i = 0
while c:
    cval = ord(c)

    if cval < 10:
        f2.write(chr(32))
    elif cval == 10:
        f2.write(chr(13))
    elif cval <= 32:
        f2.write(chr(32))
    elif cval < 65:
        f2.write(chr(cval))
    elif cval < 91:
        f2.write(chr(cval+128))
    elif cval < 97:
        f2.write(chr(cval))
    elif cval < 123:
        f2.write(chr(cval-32))
    elif cval < 128:
        f2.write(chr(cval+96))

    c = f.read(1)

f.close
f2.close