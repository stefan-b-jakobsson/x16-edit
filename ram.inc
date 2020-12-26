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

.import __ZEROPAGE_LOAD__, __ZEROPAGE_SIZE__, __VARS_LOAD__

;******************************************************************************
;Function name.......: ram_backup
;Purpose.............: Backup of zero page addresses and $0400-07FF used by the
;                      the program to banked RAM, so that the content of those
;                      addresses can be restored on program exit
;Preparatory routines: Before calling this function, the values of mem_start
;                      and mem_top to be backed up has to be pushed onto 
;                      the stack. This function will remove those values from 
;                      the stack, but leave the JSR return address unaffected.
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc ram_backup
    lda mem_start
    sta BNK_SEL

    ;Copy zero page addresses used by the program
    backup = $b000
    VARS = __VARS_LOAD__
    zpsize = $14

    ldx #0
:   lda __ZEROPAGE_LOAD__,x
    sta $b000,x
    inx
    cpx #zpsize
    bne :-

    ;Copy VARS segment, mem_start and mem_top already affected by startup, but we
    ;handle those two bytes in a jiffy
    
    ldx #0
:   lda VARS,x
    sta backup+zpsize,x
    lda VARS+$0100,x
    sta backup+$0100+zpsize,x
    lda VARS+$0200,x
    sta backup+$0200+zpsize,x
    lda VARS+$0300,x
    sta backup+$0300+zpsize,x
    
    inx
    bne :-

    ;Get initial values of mem_start and mem_top from stack, stored there at beginning of main function
    plx     ;First pulling off return address...
    ply     ;...two bytes, and temporarily store in X and Y

    pla     ;Pull off mem_top
    sta backup+zpsize+mem_top-VARS
    pla     ;Pull off mem_start
    sta backup+zpsize+mem_start-VARS

    ;Restore return address on stack
    phy
    phx
    rts

.endproc

;******************************************************************************
;Function name.......: ram_restore
;Purpose.............: Restores zero page addresses and $0400-07FF from the
;                      backup stored by ram_backup. This function must only
;                      be called immediately before exiting the program,
;                      as most variables and buffers used by the program
;                      are overwritten.
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc ram_restore
    lda mem_start
    sta BNK_SEL

    ;Restore zero page
    zpsize = $14
    backup = $b000
    VARS = __VARS_LOAD__

    ldx #0
:   lda backup,x
    sta __ZEROPAGE_LOAD__,x
    inx
    cpx #zpsize
    bne :-

    ;Restore $0400-$07FF
    ldx #0
:   lda backup+zpsize,x
    sta VARS,x
    lda backup+zpsize+$100,x
    sta VARS+$0100,x
    lda backup+zpsize+$200,x
    sta VARS+$0200,x
    lda backup+zpsize+$300,x
    sta VARS+$0300,x
    inx
    bne :-

    rts

.endproc