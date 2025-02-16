.DATA
MyStruc Struc
db 1
MyStruc ENDS

MyStruc <1 DUP (1)>


.CODE
push [eax]
mov [eax], 1


END