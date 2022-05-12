    AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8
    EXPORT atomic_inc
    EXPORT atomic_dec

atomic_inc ; (uint32_t* num)
    MOV R1, R0
spin
    LDREX R0, [R1]
    ADD R0, R0, #1
    STREX R2, R0, [R1]
	CMP     R2, #0
    BNE     spin
    BX LR

atomic_dec ; (uint32_t* num)
    MOV R1, R0
spin_dec
    LDREX R0, [R1]
    SUB R0, R0, #1
    STREX R2, R0, [R1]
	CMP     R2, #0
    BNE     spin_dec
    BX LR

    ALIGN
    END