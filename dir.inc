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
;                      Last error state in file_ioerr
.proc dir_open
    ;Set default = no kernal error
    stz file_io_err

    ;Close file #1, and open file #1
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_SETNAM
    lda #'$'
    sta dirpath
    lda #1
    ldx #<dirpath
    ldy #>dirpath
    bridge_call KERNAL_SETNAM

    bridge_setaddr KERNAL_SETLFS
    lda #1
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
    ldx #1
    bridge_call KERNAL_CHKIN
    bcc :+
    jmp io_error

:   clc
    rts

io_error:
    sta file_io_err
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
;Returns.............: C=1 if reached end of file or an error
;Error returns.......: Call file_read_disk_status to verify if
;                      a disk error occured
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

    ;A state machine parsing a line of dir data

    ldy state
    cpy #2
    bcc state0      ;State 0-1: Skip first two chars, contains pointer to start of next basic line which is of no interest to us
    cpy #2
    beq state2      ;State 2: Set size low byte (3rd char on line)
    cpy #3
    beq state3      ;State 3: Set size high byte (4th char on line)
    cpy #4
    beq state4      ;State 4: Search for double quote, marking the start of the file/dir name
    cpy #5
    beq state5      ;State 5: Search for double quote, marking the end of the file/dir name
    cpy #6
    beq state6      ;State 6: Search for first non blank space char and save it as file type (eg d=dir, p=prg ...)
    jmp state7      ;State 7: Search for null char terminating the line, and return

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
    lda dir_type
    beq exit            ;An item with no type info (dir_type=0) means that we have read the last item of the directory (the blocks free summary)

    clc
    rts

eof:
    pla                 ;Restore stack

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
    lda #1
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
;Error returns.......: C=1 if Kernal error
;                      Call file_read_disk_status to verify if
;                      a disk error occured
.proc dir_getitem
    stx skip
    sty skip+1

    jsr dir_open
    bcs err

loop:
    jsr dir_next

    lda skip
    bne :+
    lda skip+1
    beq exit
    
:   lda skip
    bne :+
    dec skip+1
:   dec skip
    bra loop

exit:
    jsr dir_close
    clc
    rts

err:
    rts

.segment "VARS"
    skip: .res 2
.CODE
.endproc

;******************************************************************************
;Function name.......: dir_show
;Purpose.............: Displays one page of directory entries on the screen
;Input...............: Ordinal number (16 bit index) of the the first entry
;                      that is shown, X=LSB, Y=MSB
;Returns.............: C=1 reached end of file or error
;Error returns.......: 
.proc dir_show
    stz dir_itemcount
    stz dir_islastpage
    
    stx dir_first
    sty dir_first+1

    stx skip
    sty skip+1
    
    jsr dir_open
    bcc prepare
    
err:
    jsr screen_clear_footer
    jsr screen_print_default_footer
    jsr cursor_activate
    jsr prompt_close
    lda #2
    sta APP_MOD

    ldx file_io_err
    lda file_ioerr_H,x
    tay
    lda file_ioerr_L,x
    tax
    jmp screen_print_status

prepare:
    jsr screen_clear

    stz VERA_L
    lda #3
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

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

    jsr file_read_disk_status
    cmp #0
    beq :+

    ldx #<file_disk_status
    ldy #>file_disk_status
    jsr screen_print_status

:   ldx #0
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
;Function name.......: dir_change
;Purpose.............: Change current directory
;Input...............: Pointer to directory name, X=LSB, Y=MSB
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
    dir_itemcount: .res 1
    dir_islastpage: .res 1
.CODE