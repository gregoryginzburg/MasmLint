INCLUDE settings.inc
INCLUDE io2023.inc

    mov eax, 1

.STACK 2

.DATA
    helloMessage db "Hello, world!", [1]
    test1 db 2 dup (<1, 2, 3)
    var db 1
    test db 1
    awd <1, 2>
    a DUP
    b 1 DUP 
    c (1 + 2) DUP (1, 2
    db 1
    byte dw 3

.CODE
    a1 STRUC
        db 1
    a1 ENDS

    a2 STRUC
        db 1
    b ENDS

    a STRUC
        db 1
    ENDS

    STRUC

    adwa RECORD anothersym:321, 


start:
    ; fixed things
    mov eax, ebx + 1

    mov eax, var[eax + eax]

    mov eax, var[eax * 2 + 5]

    mov eax, [esp + esp]

    mov eax, [esp][esp]

    mov eax, [ax]

    mov eax, x[ebx].a[esi]

    inchar al
; END start

