INCLUDE settings.inc
INCLUDE io2023.inc

.DATA
    b db 1
    r dd 2
    n db -1
    c dd 534
    helloMessage db "Hello, world!", 0
    test1 db "awgwa"
.CODE

func PROC
    push ebp
    mov ebp, esp

    MOV AL, 0FFh
    ADD AL, 1

    MOV AL, 127
    ADD AL, 1 

    MOV AL, 5
    SUB AL, 5 

    mov esp, ebp
    pop ebp
    ret
func ENDP

start:
    ; mov dword ptr [ds:0], eax 
    push ebp
    mov ebp, esp
    mov EAX, -2
    ; MOV BYTE PTR [5678], CL
    mov dword ptr [cs:0], ebx
    mov r, ebx
    CDQ
    mov ebx, 1
    ; div bx
    ; int 3
    push offset cs:b
    call func

    OUTSTR offset helloMessage

    jmp test_label
    mov ebx, 2
    mov ebx, 3


test_label:
    mov ecx, 3
    jmp test_label
    
    EXIT
END start