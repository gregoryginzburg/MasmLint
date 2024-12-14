
MyStruc STRUC
MyStruc ENDS
.DATA
var dd 4

.CODE
start:
mov eax, TYPE [eax]
END