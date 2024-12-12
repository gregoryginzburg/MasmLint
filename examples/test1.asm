.DATA

var dd 2 dup(1, 2)

E STRUC
    ; E <>
    ; a db 2
E ENDS


x E <>

S STRUC
    db byte PTR 1
    E <>
    ; dd 1 dup (2, 3)
S ENDS


; S <1 dup (2), 1 dup (2)>


.CODE
    mov eax, E PTR [eax]
    mov eax, 1 / (sizeof var - 16)
    ; mov byte PTR [eax], 10000000000

END