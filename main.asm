;******************************************************************************
;Copyright 2020-2021, Stefan Jakobsson.
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

.export main_default_entry
.export main_loadfile_entry

;******************************************************************************
;Check build target
.define target_ram 1
.define target_rom 2

.ifndef target_mem
    .error "target_mem not set (1=RAM, 2=ROM)"
.elseif target_mem=1
.elseif target_mem=2
.else
    .error "target_mem invalid value (1=RAM, 2=ROM)"
.endif

;******************************************************************************
;Include global defines
.include "common.inc"
.include "charset.inc"
.include "bridge_macro.inc"

;******************************************************************************
;Description.........: Entry points
jmp main_default_entry
jmp main_loadfile_entry

;******************************************************************************
;Function name.......: main_default_entry
;Purpose.............: Default entry function; starts the editor with an
;                      empty buffer
;Input...............: First RAM bank used by the program in X and last RAM
;                      bank used by the program in Y.
;Returns.............: Nothing
;Error returns.......: None
.proc main_default_entry
    jsr main_init
    bcs exit
    jmp main_loop
exit:
    rts
.endproc

;******************************************************************************
;Function name.......: main_loadfile_entry
;Purpose.............: Program entry function
;Input...............: First RAM bank used by the program in X and last RAM
;                      bank used by the program in Y
;Returns.............: Nothing
;Error returns.......: None
.proc main_loadfile_entry
    jsr main_init
    bcs exit
    ldx r0
    ldy r0+1
    lda r1
    jsr cmd_file_open
    
    ldx #0
    ldy #2
    jsr cursor_move

    jmp main_loop
exit:
    rts
.endproc

;******************************************************************************
;Function name.......: main_init
;Purpose.............: Initializes the program
;Input...............: First RAM bank used by the program in X and last RAM
;                      bank used by the program in Y. If building the RAM version
;                      the values are ignored and replaced with X=1 and Y=255.
;Returns.............: C=1 if program initialization failed
;Error returns.......: None
.proc main_init
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

    ;Check if banked RAM start<=top
    ldy mem_top
    cpy mem_start
    bcs rambackup

    ;Error: mem_top < mem_start
    bridge_setaddr KERNAL_CHROUT
    ldx #0
print_error:
    lda errormsg,x
    beq print_error_done
    bridge_call KERNAL_CHROUT
    inx
    bra print_error

print_error_done:
    sec
    rts

    ;Backup ZP and low RAM, so we can restore on program exit
rambackup:
    jsr ram_backup

    ;Set program mode to default
    stz APP_MOD

    ;Copy Kernal bridge code to RAM
    .if (::target_mem=target_rom)
        jsr bridge_copy
    .endif

    ;Initialize base functions
    jsr mem_init
    jsr file_init
    jsr screen_init
    jsr cursor_init
    jsr clipboard_init
    jsr cmd_init
    jsr scancode_init
    
    clc
    rts

errormsg:
    .byt "error: banked ram top less than banked ram start",0
.endproc

;******************************************************************************
;Function name.......: main_loop
;Purpose.............: Initializes custom interrupt handler and then goes
;                      to the program main loop that just waits for program
;                      to terminate.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc main_loop
    ;Init IRQ
    jsr irq_init

    ;Set program in running state
    stz APP_QUIT

    ;Wait for the program to terminate    
:   lda APP_QUIT        ;0=running, 1=closing down, 2=close now
    cmp #2
    bne :-

    jsr scancode_restore
    jsr ram_restore     ;Restore ZP and low RAM used by the program

exit:
    rts
.endproc

.include "appversion.inc"
.include "screen.inc"
.include "keyboard.inc"
.include "cmd.inc"
.include "cmd_file.inc"
.include "prompt.inc"
.include "irq.inc"
.include "scancode.inc"
.include "cursor.inc"
.include "file.inc"
.include "util.inc"
.include "clipboard.inc"
.include "mem.inc"
.include "ram.inc"
.include "dir.inc"

.if target_mem=target_rom
    .include "bridge.inc"
.endif