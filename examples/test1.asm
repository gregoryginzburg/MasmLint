.DATA

; x db 3 dup (1, 2, 3)
x dd 1

.CODE
    push [eax]
    push byte ptr [eax]
    push ax
    push ffffffffffh
    push eax
END
