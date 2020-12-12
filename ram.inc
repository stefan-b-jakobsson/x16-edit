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
    zpstart = CRS_BNK
    zpsize = CRS_Y-zpstart+1
    backup = $b000

    ldx #0
:   lda zpstart,x
    sta $b000,x
    inx
    cpx #zpsize
    bne :-

    ;Copy $0400-$07FF, mem_start and mem_top already affected by startup, but we
    ;handle those two bytes in a jiffy
    ldx #0
:   lda $0400,x
    sta backup+zpsize,x
    lda $0500,x
    sta backup+$100+zpsize,x
    lda $0600,x
    sta backup+$200+zpsize,x
    lda $0700,x
    sta backup+$300+zpsize,x
    inx
    bne :-

    ;Get initial values of mem_start and mem_top from stack, stored there at beginning of main function
    plx     ;First pulling off return address...
    ply     ;...two bytes, and temporarily store in X and Y

    pla     ;Pull off mem_top
    sta backup+zpsize+mem_top-$0400
    pla     ;Pull off mem_start
    sta backup+zpsize+mem_start-$0400

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
    zpstart = CRS_BNK
    zpsize = CRS_Y-zpstart+1
    backup = $b000

    ldx #0
:   lda backup,x
    sta zpstart,x
    inx
    cpx #zpsize
    bne :-

    ;Restore $0400-$07FF
    ldx #0
:   lda backup+zpsize,x
    sta $0400,x
    lda backup+zpsize+$100,x
    sta $0500,x
    lda backup+zpsize+$200,x
    sta $0600,x
    lda backup+zpsize+$300,x
    sta $0700,x
    inx
    bne :-

    rts

.endproc