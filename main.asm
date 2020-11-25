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

;Target
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

    ;Set program mode to default
    stz APP_MOD

    ;Set program in running state
    stz APP_QUIT

    ;Copy Kernal bridge code to RAM $0780
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
    jsr irq_init
    
    ;Wait for the program to terminate    
:   lda APP_QUIT        ;0=running, 1=closing down, 2=close now
    cmp #2
    bne :-
    
    rts
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

.if target_mem=target_rom
    .include "bridge.inc"
.endif