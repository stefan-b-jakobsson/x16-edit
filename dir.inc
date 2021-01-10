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

.define dir_entries_per_page 50

;******************************************************************************
;Function name.......: dir_open
;Purpose.............: Opens file for reading directory
;Input...............: None
;Returns.............: Noting
;Error returns.......: On error C=1
;                      Last error state in dir_ioerr
.proc dir_open
    ;Set default = no kernal error
    stz file_io_err

    ;Close file #1, and open file #1
    bridge_setaddr KERNAL_CLOSE
    lda #14
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_SETNAM
    lda #'$'
    sta dirpath
    lda #1
    ldx #<dirpath
    ldy #>dirpath
    bridge_call KERNAL_SETNAM

    bridge_setaddr KERNAL_SETLFS
    lda #14
    ldx file_cur_device
    ldy #0
    bridge_call KERNAL_SETLFS
    bcc :+
    jmp io_error

:   bridge_setaddr KERNAL_OPEN
    bridge_call KERNAL_OPEN
    bcc :+
    jmp io_error

:   bridge_setaddr KERNAL_CHKIN
    ldx #14
    bridge_call KERNAL_CHKIN
    bcc :+
    jmp io_error

:   clc
    rts

io_error:
    sta dir_ioerr
    sec
    rts

.segment "VARS"
    dirpath: .res 1
.CODE
.endproc

;******************************************************************************
;Function name.......: dir_next
;Purpose.............: Retrieves next directory entry from file system
;Input...............: None
;Returns.............: filename in          dir_entry
;                      filename length in   dir_filename_len
;                      file type in         dir_type
;                      file block size in   dir_size
;Error returns.......: On error C=1
;                      Last error state in dir_ioerr
.proc dir_next
    stz state
    stz dir_filename_len
    stz dir_type

    ldy mem_start
    sty BNK_SEL

read_loop:
    bridge_setaddr KERNAL_CHRIN
    bridge_call KERNAL_CHRIN

    pha

    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    cmp #0
    beq :+
    jmp eof

:   pla

    ldy state
    cpy #2
    bcc state0
    cpy #2
    beq state2
    cpy #3
    beq state3
    cpy #4
    beq state4
    cpy #5
    beq state5
    cpy #6
    beq state6
    jmp state7

state0:
    inc state
    jmp read_loop

state2:
    sta dir_size
    inc state
    jmp read_loop

state3:
    sta dir_size+1
    inc state
    jmp read_loop

state4:
    cmp #$22
    bne read_loop
    inc state
    jmp read_loop

state5:
    cmp #$22
    bne :+

    inc state
    jmp read_loop

:   ldy dir_filename_len
    cpy #255
    beq :+
    
    sta dir_entry,y
    inc dir_filename_len

:   jmp read_loop

state6:
    cmp #$20
    bne :+
    jmp read_loop

:   sta dir_type
    inc state
    jmp read_loop

state7:
    cmp #0
    beq endofitem
    jmp read_loop

endofitem:
    lda dir_entry
    cmp #'$'
    bne :+
    jmp dir_next

:   lda dir_type
    beq exit

    clc
    rts

eof:
    pla

exit:
    jsr dir_close
    lda #1
    sta dir_islastpage
    sec
    rts

.segment "VARS"
    state: .res 1
.CODE
.endproc

;******************************************************************************
;Function name.......: dir_close
;Purpose.............: Closes file used for reading directory entries
;Input...............: None
;Returns.............: Nothing
;Error returns.......: Nothing
.proc dir_close
    bridge_setaddr KERNAL_CLOSE
    lda #14
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN
    rts
.endproc

;******************************************************************************
;Function name.......: dir_getitem
;Purpose.............: Reads data for one directory entry
;Input...............: Ordinal number (16 bit index) of the the entry
;                      X=LSB, Y=MSB
;Returns.............: filename in          dir_entry
;                      filename length in   dir_filename_len
;                      file type in         dir_type
;Error returns.......: Last error state in dir_ioerr
.proc dir_getitem
    stx skip

    jsr dir_open

:   jsr dir_next
    lda skip
    beq :+
    dec skip
    jmp :-

:   jmp dir_close

.segment "VARS"
    skip: .res 1
.CODE
.endproc

;******************************************************************************
;Function name.......: dir_show
;Purpose.............: Displays one page of directory entries on the screen
;Input...............: Ordinal number (16 bit index) of the the first entry
;                      that is shown, X=LSB, Y=MSB
;Returns.............: C=1 reached end of file or error
;Error returns.......: Last error state in dir_ioerr
.proc dir_show
    stz dir_itemcount
    stz dir_islastpage
    
    stx dir_first
    sty dir_first+1

    stx skip
    sty skip+1

    jsr screen_clear

    stz VERA_L
    lda #3
    sta VERA_M
    lda #(2<<4)
    sta VERA_H
    
    jsr dir_open
    bcc skip_loop
    
    rts

skip_loop:
    lda skip
    bne :+
    lda skip+1
    bne :+
    jmp loop

:   jsr dir_next
    bcc :+
    jmp endmsg
:   dec skip
    lda skip
    cmp #$ff
    bne skip_loop
    dec skip+1
    jmp skip_loop

loop:
    jsr dir_next
    bcc print_dir
    jmp endmsg
    
print_dir:
    lda dir_type
    cmp #'d'
    bne print_size

    ldy #0
:   lda dirlbl,y
    beq print_filename
    jsr screen_put_uc_char
    iny
    jmp :-

print_size:
    lda #6*2+4*2
    sta VERA_L
    
    ldx dir_size
    ldy dir_size+1
    lda #0
    jsr util_bin_to_bcd
    jsr util_bcd_to_str
    stx TMP1_ADR
    sty TMP1_ADR+1
    jsr util_strlen
    dey
    
    lda #%00101000
    sta VERA_H

:   lda (TMP1_ADR),y
    beq print_filename
    sta VERA_D0
    dey
    cpy #$ff
    bne :-

print_filename:
    lda #(2<<4)
    sta VERA_H
    lda #12*2
    sta VERA_L

    ldy #0
:   cpy dir_filename_len
    beq nextitem
    lda dir_entry,y
    jsr screen_put_char
    iny
    cpy #80-12
    bne :-

nextitem:
    inc VERA_M
    stz VERA_L

    inc dir_itemcount
    lda dir_itemcount
    cmp #dir_entries_per_page
    bcs more
    jmp loop

more:
    stz VERA_L
    inc VERA_M

    ldy #$ff
:   iny
    lda morelbl,y
    beq exit
    jsr screen_put_uc_char
    bra :-

endmsg:
    stz VERA_L
    inc VERA_M
    ldy #$ff
:   iny
    lda nomorelbl,y
    beq exit
    jsr screen_put_uc_char
    bra :-

exit:
    jsr dir_close

    ldx #0
    jmp dir_select

.segment "VARS"
    skip: .res 2
.CODE

dirlbl:
    .byt "<dir>",0
morelbl:
    .byt "--- more ---",0
nomorelbl:
    .byte "--- end ---",0
.endproc

;******************************************************************************
;Function name.......: dir_select
;Purpose.............: Selects and marks one on screen directory entry
;Input...............: X=Ordinal number (8 bit index) of the entry
;Returns.............: Nothing
;Error returns.......: Nothing
.proc dir_select
    cpx dir_itemcount
    bcc :+
    rts
    
:   phx

    lda #1
    sta VERA_L
    clc
    lda #3
    adc dir_selected
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldx #80
:   lda screen_color
    sta VERA_D0
    dex
    bne :-

    lda #1
    sta VERA_L
    pla
    sta dir_selected
    clc
    adc #3
    sta VERA_M

    lda screen_color
    asl
    adc #$80
    rol
    asl
    adc #$80
    rol

    ldx #80
:   sta VERA_D0
    dex
    bne :-

    rts
.endproc

;******************************************************************************
;Function name.......: dir_nextpage
;Purpose.............: Shows next page of the directory
;Input...............: None
;Returns.............: Nothing
;Error returns.......: Nothing
.proc dir_nextpage
    lda dir_islastpage
    beq :+
    rts

:   clc
    lda dir_first
    adc #dir_entries_per_page
    tax
    lda dir_first+1
    adc #0
    tay
    jmp dir_show
.endproc

;******************************************************************************
;Function name.......: dir_prevpage
;Purpose.............: Shows previous page of the directory
;Input...............: None
;Returns.............: Nothing
;Error returns.......: Nothing
.proc dir_prevpage
    lda dir_first
    cmp #2
    bcs :+
    rts

:   sec
    lda dir_first
    sbc #dir_entries_per_page
    tax
    lda dir_first+1
    sbc #0
    tay
    jmp dir_show
.endproc

;******************************************************************************
;Function name.......: dir_select
;Purpose.............: Selects and marks one on screen directory entry
;Input...............: X=Ordinal number (8 bit index) of the entry
;Returns.............: Nothing
;Error returns.......: Nothing
.proc dir_change
    pha

    stx TMP1_ADR
    sty TMP1_ADR+1
    clc
    txa
    adc #3
    sta TMP2_ADR
    tya
    adc #0
    sta TMP2_ADR+1

    ldy #252
:   lda (TMP1_ADR),y
    sta (TMP2_ADR),y
    dey
    cpy #$ff
    bne :-

    ldy #0
    lda #'c'
    sta (TMP1_ADR),y
    iny
    lda #'d'
    sta (TMP1_ADR),y
    iny
    lda #':'
    sta (TMP1_ADR),y

    clc
    pla
    adc #3
    ldx TMP1_ADR
    ldy TMP1_ADR+1
    jsr file_disk_cmd
    
    ldx #1
    ldy #0
    jmp dir_show 
.endproc

.segment "VARS"
    dir_first: .res 2
    dir_selected: .res 1
    dir_filename_len: .res 1
    dir_type: .res 1
    dir_size: .res 2
    dir_ioerr: .res 1
    dir_itemcount: .res 1
    dir_islastpage: .res 1
.CODE