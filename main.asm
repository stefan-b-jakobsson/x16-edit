;******************************************************************************
;Copyright 2020, Stefan Jakobsson.
;
;This file is part of X16 Edit.
;
;X16 Edit is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.
;
;X16 Edit is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with X16 Edit.  If not, see <https://www.gnu.org/licenses/>.
;******************************************************************************

;Build target
.define target_ram 1
.define target_rom 2

.ifndef target_mem
    .error "target_mem not set (1=RAM, 2=ROM)"
.elseif target_mem=1
.elseif target_mem=2
.else
    .error "target_mem invalid value (1=RAM, 2=ROM)"
.endif


;Include global defines
.include "common.inc"
.include "charset.inc"
.include "bridge_macro.inc"

;******************************************************************************
;Function name.......: main
;Purpose.............: Program entry function
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc main
    ;Ensure we are in binary mode
    cld

    ;Save content of mem_start and mem_top on stack before the values are changed
    ;These values are to be picked up by the ram_backup function
    ;Not the cleanest solution, but there's nothing else to do if RAM is to be preserved
    lda mem_start
    pha
    lda mem_top
    pha

    ;Set banked RAM start and end
    .if (::target_mem=target_ram)
        ldx #1
        ldy #255
    .endif
    stx mem_start
    sty mem_top

    cpy mem_start
    bcs rambackup

;Error: mem_top < mem_start => exit program
    bridge_setaddr KERNAL_CHROUT
    ldx #0
printerrloop:
    lda errormsg,x
    beq exit
    bridge_call KERNAL_CHROUT
    inx
    jmp printerrloop

    ;Backup ZP page and $0400-$07FF data so it can be restored on program exit
rambackup:
    jsr ram_backup

    ;Copy Kernal bridge code to RAM
    .if (::target_mem=target_rom)
        jsr bridge_copy
    .endif

    ;Set program mode to default
    stz APP_MOD

    ;Set program in running state
    stz APP_QUIT

    ;Initialize base functions
    jsr mem_init
    jsr file_init
    jsr screen_init
    jsr cursor_init
    jsr clipboard_init
    jsr cmd_init
    jsr irq_init
    
    ;Wait for the program to terminate    
:   lda APP_QUIT        ;0=running, 1=closing down, 2=close now
    cmp #2
    bne :-

    jsr ram_restore     ;Restore ZP and $0400-$07FF
exit:
    rts

errormsg:
    .byt "error: banked ram top less than banked ram start",0
.endproc

.include "screen.inc"
.include "keyboard.inc"
.include "cmd.inc"
.include "cmd_file.inc"
.include "prompt.inc"
.include "irq.inc"
.include "cursor.inc"
.include "file.inc"
.include "util.inc"
.include "clipboard.inc"
.include "mem.inc"
.include "ram.inc"

.if target_mem=target_rom
    .include "bridge.inc"
.endif