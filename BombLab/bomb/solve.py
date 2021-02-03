str1 = "Border relations with Canada have never been better."

str2 = "1 2 4 8 16 32"

str3 = "7 327"

str4 = "1 0"

str5 = "ionefg"

str6 = "4 3 2 1 6 5"

print(str1)
print(str2)
print(str3)
print(str4)
print(str5)
print(str6)

def dep5():
    arr = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
    brr = []
    target = "flyers"
    for i in target:
        brr.append(arr.index(i))

    for i in brr:
        print(chr(0x60 + i), end="")
    print("")
    for i in brr:
        print(chr(0x40 + i), end="")
    print("")
    for i in brr:
        print(chr(0x30 + i), end="")
    print("")
