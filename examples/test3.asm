.DATA
    var db EAX
    var = 1

    anothervar db 2
    mov db 1

    var1 = 2
    var1 = 5
    var1 = anothervar

    var1 = var2
    var2 = 2

MyStruc STRUC
    a db 1
    a db 2
MyStruc ENDS

    db 1
    db 257
    db -129
    db "waddddddawddw"
    db anothervar DUP (2)

    MyStruc <>
    MyStruc <1, 2>
    anothervar 2
    MyStruc <2>
    MyStruc < <3> >
    MyStruc <1 dup (2)>

    MyRec <>


MyRec RECORD x0:32
MyRec1 RECORD x1:31, x2:31
MyRec2 RECORD x3:-1

AnotherStruc STRUC
    a db 1
AnotherStruc ENDS


x AnotherStruc <>

.CODE
start:
    mov eax, AnotherStruc PTR [eax]
    mov eax, x[ebx].a[esi]

    mov eax, 1 / var5
    var5 = 0

    mov [eax].s, 1

    mov (BYTE PTR [eax]).s, 1

    mov (MyStruc PTR [eax]).e, 1
    mov (MyStruc PTR [eax]).a, 1


    mov eax, MyRec
    mov eax, MyStruc

    mov eax, anothervar PTR [eax]
    
    mov eax, eax, eax

    mov [eax], [eax]

    mov 1, eax

    mov al, 255
    mov al, 256
    mov al, -128
    mov al, -129

    mov byte PTR [eax], 10000000000

    mov eax, anothervar

    mov eax, QWORD PTR [eax]

    mov eax, dword ptr anothervar

    mov eax, word ptr anothervar

    call [eax]

    call byte ptr [eax]

    dec byte ptr [eax]

    dec 1
    dec offset anothervar

    inchar al
    inchar eax
    inchar 2
    inchar [eax]
    inchar byte ptr [eax]



END start