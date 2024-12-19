.DATA
var1 dd 1

.CODE
    xchg [eax], [eax]
    xchg eax, 1
    xchg eax, al
    xchg eax, [ebx]


END