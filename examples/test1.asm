.DATA

var dd 1

E STRUC
    a db 257
E ENDS


x E <>

S STRUC
    db 1
    E <>
    dd 1 dup (2, 3)
S ENDS
    
; S <1 dup (2), 1 dup (2)>


.CODE
    mov byte PTR [eax], 10000000000

END