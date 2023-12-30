.align 2

.global _set_x21
_set_x21:
    mov		x21, x0
    ret

.global _set_x22
_set_x22:
    mov		x22, x0
    ret

.global _set_x8
_set_x8:
    mov		x8, x0
    ret

.global _set_x0
_set_x0:
    mov		x0, x0
    ret
	
.global _set_lr
_set_lr:
    mov		lr, x0
    ret
	
.global _set_fp
_set_fp:
    mov		fp, x0
    ret
	
.global _set_sp
_set_sp:
    mov		sp, x0
    ret
