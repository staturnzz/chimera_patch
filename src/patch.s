.align 2

// start at 0x10000DEB8
_patch:
    adrp    x3, #0x4e000            // set x3 to 0x10005B000
    add     x4, x3, #0xa48          // add offset to tfp0 into x4
    add     x21, x3, #0x778         // add offset to kernel_slide into x21

    // kernel_slide temporarily stores our replacment exploit func ptr,
    // we store said ptr when ctor is called

    ldr     x6, [x21]               // load func ptr into x6
    mov     x8, #-1                 
    str     x8, [x21]               // now set kernel_slide value to -1

    ldr     x8, [x19, #0x38]    
    ldr     x8, [x8, #8]
    strb    wzr, [x8, #0x18]

    mov     x0, x4                  // move tfp0 ptr into x0 (arg0)
    mov     x1, x21                 // move kernel_slide ptr into x1 (arg1)

    blr     x6                     // branch to the replace exploit func
// end at 0x10000DEE8


// start at 0x10000DEB8
_orig:
    adr             x21, kernel_base
    nop

    mov             x8, #-1
    str             x8, [x21]
    ldr             x8, [x19,#0x38]
    ldr             x8, [x8,#8]
    strb            wzr, [x8,#0x18]

    adr             x22, tfp0
    nop

    mov             x0, x22
    mov             x1, x21
    bl              life_waste
// end at 0x10000DEE8
