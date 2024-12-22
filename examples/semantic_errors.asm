INCLUDE settings.inc
INCLUDE io2023.inc

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

    dd 0fffffffffh
    dd 0ffffffffh + 1
    dq +0fffffffffffffffffh
    dq 0fffffffffffffffffh

    db "1"
    db "12"
    db "123"
    db "1234"
    db "12345"

    dw "1"
    dw "12"
    dw "123"
    dw "1234"
    dw "12345"

    dd "1"
    dd "12"
    dd "123"
    dd "1234"
    dd "12345"

    dq "1"
    dq "12"
    dq "123"
    dq "1234"
    dq "12345"


MyStruc STRUC
    a db 1
    ; a db 2
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
    MyStruc 1
    MyStruc <1 dup (2)>

    MyRec <>


    MyRec RECORD x0:32
    MyRec1 RECORD x1:31, x2:31
    MyRec2 RECORD x3:-1

AnotherStruc STRUC
    a db 1 dup (2)
AnotherStruc ENDS


x AnotherStruc <>

var6 dd 1

.CODE
start:
    mov al, byte ptr (offset var6)

    mov eax, byte
    mov eax, AnotherStruc PTR [eax]
    mov eax, x[ebx].a[esi]

    mov eax, dword ptr [1]

    mov al, '12'

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

    dec ptr [eax]

    dec 1
    dec offset anothervar

    ja start + 1

    lea [eax], [eax]
    lea ax, [eax]
    lea eax, 2
    lea eax, [ebx]

    movsx [eax], 1
    movsx eax, 1
    movsx eax, [eax]
    movsx eax, dword ptr [eax]
    movsx eax, byte ptr [eax]

    push [eax]
    push byte ptr [eax]
    push ax
    push ffffffffffh
    push eax

    rol 1, 1
    rol [eax], 1
    rol dword ptr [eax], al
    rol dword ptr [eax], 300
    rol dword ptr [eax], 1
    rol dword ptr [eax], cl

    ret
    ret eax, ebx
    ret 0fffffh
    ret eax
    ret 2

    xchg [eax], [eax]
    xchg eax, 1
    xchg eax, al
    xchg eax, [ebx]


    inchar al
    inchar eax
    inchar 2
    inchar [eax]
    inchar byte ptr [eax]

    outi [eax]
    outi byte ptr [eax]
    outi ax
    outi ffffffffffh
    outi eax

    outchar [eax]
    outchar byte ptr [eax]
    outchar ax
    outchar ffffffffffh
    outchar eax

    NEWLINE 1
    EXIT 2


END start