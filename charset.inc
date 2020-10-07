;******************************************************************************
;Function name.......: charset_petscii_to_scrcode
;Purpose.............: Initializes the screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
;Credits.............: https://sta.c64.org/cbm64pettoscr.html
.macro charset_petscii_to_scrcode
    cmp #32
    bcc charset_petscii_to_scrcode_add_128
    cmp #64
    bcc charset_petscii_to_scrcode_exit
    cmp #96
    bcc charset_petscii_to_scrcode_sub_64
    cmp #128
    bcc charset_petscii_to_scrcode_sub_32
    cmp #160
    bcc charset_petscii_to_scrcode_add_64
    cmp #192
    bcc charset_petscii_to_scrcode_sub_64
    cmp #255
    bcc charset_petscii_to_scrcode_sub_128

    lda #94
    jmp charset_petscii_to_scrcode_exit

charset_petscii_to_scrcode_add_128:
    clc
    adc #128
    jmp charset_petscii_to_scrcode_exit

charset_petscii_to_scrcode_sub_32:
    sec
    sbc #32
    jmp charset_petscii_to_scrcode_exit

charset_petscii_to_scrcode_add_64:
    clc
    adc #64
    jmp charset_petscii_to_scrcode_exit

charset_petscii_to_scrcode_sub_64:
    sec
    sbc #64
    jmp charset_petscii_to_scrcode_exit

charset_petscii_to_scrcode_sub_128:
    sec
    sbc #128

charset_petscii_to_scrcode_exit:
    
.endmacro

;******************************************************************************
;Function name.......: charset_ascii_to_petscii_ucase
;Purpose.............: Initializes the screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.macro charset_ascii_to_petscii_ucase
    cmp #97
    bcc charset_ascii_to_petscii_ucase_exit

    cmp #123
    bcs charset_ascii_to_petscii_ucase_exit

    sec
    sbc #32

charset_ascii_to_petscii_ucase_exit:
    charset_petscii_to_scrcode
    
.endmacro

;******************************************************************************
;Function name.......: charset_ascii_to_petscii_lcase
;Purpose.............: Initializes the screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.macro charset_ascii_to_petscii_lcase
    cmp #65
    bcc charset_ascii_to_petscii_lcase_exit

    cmp #91
    bcc charset_ascii_to_petscii_lcase_add32

    cmp #97
    bcc charset_ascii_to_petscii_lcase_exit

    cmp #123
    bcc charset_ascii_to_petscii_lcase_sub32

    jmp charset_ascii_to_petscii_lcase_exit

charset_ascii_to_petscii_lcase_add32:
    clc
    adc #32
    jmp charset_ascii_to_petscii_lcase_exit

charset_ascii_to_petscii_lcase_sub32:
    sec
    sbc #32

charset_ascii_to_petscii_lcase_exit:
    charset_petscii_to_scrcode

.endmacro