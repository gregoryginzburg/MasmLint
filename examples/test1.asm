.DATA
 anothervar db 1


.CODE
mov (BYTE PTR [eax]).s, 1
END