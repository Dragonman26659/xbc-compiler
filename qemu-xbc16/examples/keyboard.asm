.16bc
.code
mov eax handler
mov 63278 eax
mov eax 0
setirr 1
wait: bra wait
handler: hlt
