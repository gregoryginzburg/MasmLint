mov eax, 1

.DATA
    helloMessage db "Hello, world!", [1]
    test1 db 2 dup (<1, 2, 3)
    mov db 1
    awd <1, 2>
    a DUP
    b 1 DUP 
    c (1 + 2) DUP (1, 2


.CODE
a STRUC
    db 1
a ENDS

a STRUC
    db 1
b ENDS

a STRUC
    db 1
ENDS

STRUC

start:
    ; fixed things
    mov eax, ebx + 1
    mov eax, var[eax + eax]
    mov eax, var[eax * 2 + 5]
    mov eax, [esp][esp]
    mov eax, [ax]
    mov eax, x[ebx].a[esi]


END start


