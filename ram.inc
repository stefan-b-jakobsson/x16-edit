;******************************************************************************
;Copyright 2020-2022, Stefan Jakobsson.
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
;Input...............: X=mem start
;Returns.............: Nothing
;Error returns.......: None
.proc ram_backup
    stx BNK_SEL

    ;Copy golden RAM
    ldx #0
:   lda $0400,x
    sta ram_backup_storage,x
    lda $0500,x
    sta ram_backup_storage+$0100,x
    lda $0600,x
    sta ram_backup_storage+$0200,x
    lda $0700,x
    sta ram_backup_storage+$0300,x
    inx
    bne :-

    ;Copy zero page
    ldx #0
:   lda $22,x
    sta ram_backup_storage+$0400,x
    inx
    cpx #$14
    bne :-
    
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

    ;Restore golden RAM
    ldx #0
:   lda ram_backup_storage,x
    sta $0400,x
    lda ram_backup_storage+$0100,x
    sta $0500,x
    lda ram_backup_storage+$0200,x
    sta $0600,x
    lda ram_backup_storage+$0300,x
    sta $0700,x
    inx
    bne :-
    
    ;Restore zero page
    ldx #0
:   lda ram_backup_storage+$0400,x
    sta $22,x
    inx
    cpx #$14
    bne :-
    
    rts

.endproc