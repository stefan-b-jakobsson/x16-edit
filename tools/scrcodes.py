import sys

pet = 0
while pet < 256:
    if pet % 16 == 0:
        print ("\n.byt ", end="")
    else:
        print (", ", end="")

    if pet < 32:
        print(pet+128, end="")
    elif pet < 64:
        print (pet, end="")
    elif pet < 96:
        print (pet-64, end="")
    elif pet < 128:
        print (pet-32, end="")
    elif pet < 160:
        print (pet+64, end="")
    elif pet < 192:
        print (pet-64, end="")
    elif pet < 255:
        print (pet-128, end="")
    else:
        print (94, end="")
    
    pet = pet +1
print ("\n")