;*******************************************************************************
;Copyright 2022-2024, Stefan Jakobsson
;
;Redistribution and use in source and binary forms, with or without modification, 
;are permitted provided that the following conditions are met:
;
;1. Redistributions of source code must retain the above copyright notice, this 
;   list of conditions and the following disclaimer.
;
;2. Redistributions in binary form must reproduce the above copyright notice, 
;   this list of conditions and the following disclaimer in the documentation 
;   and/or other materials provided with the distribution.
;
;THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” 
;AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
;IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
;DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
;FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
;DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
;SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
;CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
;OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
;OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;*******************************************************************************

;******************************************************************************
;Function name.......: bridge_copy
;Purpose.............: Copies bridge code to RAM, necessary to make the program 
;                      run in ROM
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc bridge_copy
    ldx #bridge_end-bridge_kernal
:   lda bridge_kernal-1,x
    sta bridge_code-1,x
    dex
    bne :-
    rts
.endproc

;******************************************************************************
;Function name.......: bridge_kernal
;Purpose.............: Bridge code to be copied to RAM on program startup.
;                      Necessary to make the program run in ROM
;Input...............: Use macro BRIDGE_SETADDR to set the Kernal address
;                      to call before using this function
;Returns.............: Nothing
;Error returns.......: None

;Bridge for Kernal function calls
bridge_kernal:         
    php
    pha
    lda ROM_SEL
    sta rom_bank_save
    stz ROM_SEL        ;Kernal is ROM bank 0
    pla
    plp
    jsr $ffff          ;$ffff is just placeholder
    php
    pha
    lda rom_bank_save  ;Set ROM select to our bank again
    sta ROM_SEL
    pla
    plp
    rts                ;Size: 24 bytes

;Bridge for interrupt handling, called by the Kernal
;on interrupt, switches to X16 Edit's ROM bank and then
;calls the custom interrupt handler stored in ROM
bridge_irq:
    lda ROM_SEL
    pha
    lda rom_bank
    sta ROM_SEL
    jsr irq_handler
    pla
    sta ROM_SEL
    jmp (irq_default_handler) ;Size: 17 bytes

;Bridge for custom keyboard scan code handler
bridge_scancode:
    php
    pha

    lda ROM_SEL
    sta bridge_temp
    lda rom_bank
    sta ROM_SEL
    
    pla
    plp
    jsr scancode_handler
    php
    pha

    lda bridge_temp
    sta ROM_SEL

    pla
    plp
bridge_scancode_exit_cmd:
    rts
    .byt 0,9        ;Size: 29 bytes

bridge_end:         ;Total size: 70 bytes

.segment "VARS"
    bridge_temp:    .res 1
.CODE

.segment "IRQ"
    .byt $ff, $ff, $ff, $ff, $8b, $03
.CODE
