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

;*****************************************************************************
;KERNAL VECTORS
;*****************************************************************************

;File management
.define KERNAL_CLALL $ffe7
.define KERNAL_CLOSE $ffc3
.define KERNAL_CLRCHN $ffcc 
.define KERNAL_SETLFS $ffba
.define KERNAL_SETNAM $ffbd
.define KERNAL_OPEN $ffc0
.define KERNAL_CHKIN $ffc6
.define KERNAL_CHKOUT $ffc9
.define KERNAL_CHROUT $ffd2
.define KERNAL_CHRIN $ffcf
.define KERNAL_READST $ffb7
.define KERNAL_MEMTOP $ff99

;Other
.define KERNAL_SCREEN_SET_CHARSET $ff62
.define KERNAL_SCNKEY $FF9F
.define KERNAL_GETIN $ffe4


;*****************************************************************************
;VERA VECTORS
;*****************************************************************************
.define VERA_L $9f20
.define VERA_M $9f21
.define VERA_H $9f22

.define VERA_D0 $9f23
.define VERA_D1 $9f24

.define VERA_CTRL $9f25
.define VERA_VIDEO $9f29

.define VERA_L0_CONFIG $9f2d
.define VERA_L0_TILEBASE $9f2f

.define VERA_L1_CONFIG $9f34
.define VERA_L1_TILEBASE $9f36

;*****************************************************************************
;ZERO PAGE VECTORS
;*****************************************************************************

;Pointer to memory location: Cursor position
.define CRS_BNK $22  ;Bank
.define CRS_ADR $23  ;Address, most significant byte (MSB)
.define CRS_IDX $25  ;Index

;Pointer to memory location: First visible character of the currently edited line
.define LNV_BNK $26  ;Bank
.define LNV_ADR $27  ;Address MSB
.define LNV_IDX $29  ;Byte index

;Pointer to memory location: First visible character of the screen (top left)
.define SCR_BNK $2a  ;Bank
.define SCR_ADR $2b  ;Address MSB
.define SCR_IDX $2d  ;Index

;Temporary pointers to memory locations, used for miscellaneous (short-term) purposes
;Warning: Any routine may use these temp pointers at any time and for whatever reason
.define TMP1_BNK $2e  ;Bank
.define TMP1_ADR $2f  ;Address MSB
.define TMP1_IDX $31  ;Index

.define TMP2_BNK $32 ;Bank
.define TMP2_ADR $33 ;Address MSB
.define TMP2_IDX $35 ;Index

;Cursor screen position
.define CRS_X $36   ;Cursor screen column position
.define CRS_Y $37   ;Cursor screen row position

;*****************************************************************************
;Global variables stored in $0400
;*****************************************************************************

globalvars              = $0400

APP_MOD                 = globalvars        ;1 byte, application mode
APP_QUIT                = globalvars+1      ;1 byte, used to signal program exit

mem_modified            = globalvars+2      ;1 byte 
mem_blocks_free         = globalvars+3      ;2 bytes
mem_start               = globalvars+5      ;1 byte
mem_top                 = globalvars+6      ;1 byte
mem_cur_line            = globalvars+7      ;3 bytes
mem_cur_col             = globalvars+10      ;3 bytes

screen_mode             = globalvars+13     ;1 byte
screen_color            = globalvars+14     ;1 byte

prompt_start            = globalvars+15     ;1 byte
prompt_fvc              = globalvars+16     ;1 byte
prompt_curpos           = globalvars+17     ;1 byte
prompt_len              = globalvars+18     ;1 byte
prompt_maxlen           = globalvars+19     ;1 byte
prompt_cursor_backup    = globalvars+20     ;2 bytes

keyboard_tabwidth       = globalvars+22     ;1 byte

irq_default_handler     = globalvars+23     ;2 bytes

file_io_err             = globalvars+25     ;1 byte
file_cur_filename_len   = globalvars+26     ;1 byte
file_cur_device         = globalvars+27     ;1 byte
file_format             = globalvars+28     ;1 byte

cursor_toggle_counter   = globalvars+29     ;1 byte
cmd_auto_indent_status  = globalvars+30     ;1 byte
cmd_wordwrap_mode       = globalvars+31     ;1 byte
cmd_wordwrap_pos        = globalvars+32     ;1 byte
clipboard_pointer       = globalvars+33     ;2 bytes
clipboard_end           = globalvars+35     ;2 bytes

;*****************************************************************************
;Variables used locally by functions stored in $0400
;*****************************************************************************
tempvars                                = globalvars + 37       ;16 bytes

mem_vars                                = tempvars+16
mem_run_defrag_page                     = mem_vars              ;2 bytes
mem_delete_at_lf                        = mem_vars+2            ;1 byte
mem_crs_step_right_at_lf                = mem_vars+3            ;1 byte
mem_crs_step_left_at_lf                 = mem_vars+4            ;1 byte
mem_crs_move_to_line_start_count        = mem_vars+5            ;3 bytes
mem_crs_move_to_prev_blankspace_pagelen = mem_vars+8            ;1 byte
mem_crs_move_to_prev_blankspace_counter = mem_vars+9            ;2 bytes
mem_crs_move_to_line_end_count          = mem_vars+11           ;2 bytes
mem_lnv_step_left_backup                = mem_vars+13           ;3 bytes
mem_lnv_step_right_backup               = mem_vars+16           ;3 bytes
mem_scr_move_down_backup                = mem_vars+19           ;3 bytes
mem_scr_move_up_backup                  = mem_vars+22           ;3 bytes

screen_vars                             = mem_vars+25
screen_refresh_len                      = screen_vars           ;1 byte
screen_refresh_columns                  = screen_vars+1         ;1 byte
screen_refresh_rows                     = screen_vars+2         ;1 byte
screen_println_len                      = screen_vars+3         ;1 byte
screen_println_counter                  = screen_vars+4         ;1 byte

prompt_vars                             = screen_vars+5                    
prompt_default_input_len                = prompt_vars           ;1 byte
prompt_default_input_index              = prompt_vars+1         ;1 byte

keyboard_vars                           = prompt_vars+2
keyboard_read_and_dispatch_eventhandler = keyboard_vars         ;2 bytes
keyboard_cmd_pageup_linecount           = keyboard_vars+2       ;1 byte
keyboard_cmd_pagedown_linecount         = keyboard_vars+3       ;1 byte

file_vars                               = keyboard_vars+4
file_read_first_char                    = file_vars             ;1 byte
file_read_curpage_len                   = file_vars+1           ;1 byte
file_read_newpage                       = file_vars+2           ;2 bytes

cmd_vars                                = file_vars+4
cmd_word_wrap_prevblank                 = cmd_vars              ;2 bytes
cmd_insert_lf_counter1                  = cmd_vars+2            ;2 bytes
cmd_insert_lf_counter2                  = cmd_vars+4            ;2 bytes
cmd_insert_tab_counter                  = cmd_vars+6            ;3 bytes
cmd_delete_counter                      = cmd_vars+9            ;2 bytes
cmd_go_up_count                         = cmd_vars+11           ;2 bytes
cmd_go_down_count                       = cmd_vars+13           ;2 bytes
cmd_go_end_counter                      = cmd_vars+15           ;2 bytes
cmd_show_curpos_index                   = cmd_vars+17           ;1 byte
cmd_do_find_string_len                  = cmd_vars+18           ;1 byte
cmd_do_find_match                       = cmd_vars+19           ;1 byte
cmd_replace_searchfor_len               = cmd_vars+20           ;3 bytes
cmd_replace_counter                     = cmd_vars+23           ;1 byte
cmd_replace_replace_all                 = cmd_vars+24           ;1 byte
cmd_replace_replaced_count              = cmd_vars+25           ;3 bytes
cmd_do_goto_line_line                   = cmd_vars+28           ;3 bytes
cmd_set_word_wrap_col                   = cmd_vars+31           ;4 bytes
cmd_display_msg                         = cmd_vars+35           ;59 bytes, used as temp storage of various messages

cmd_file_vars                           = cmd_vars+94
cmd_file_do_set_device_promptval        = cmd_file_vars         ;3 bytes

clipboard_vars                          = cmd_file_vars+3
clipboard_copy_count                    = clipboard_vars        ;3 bytes
clipboard_cut_count                     = clipboard_vars+3      ;3 bytes

;*****************************************************************************
;Buffers
;*****************************************************************************
prompt_input                            = $0500                 ;256 bytes
file_cur_filename                       = $0600                 ;256 bytes
file_disk_status                        = $0700                 ;127 bytes
bridge_code                             = $0780                 

;*****************************************************************************
;VARIABLES STORED IN RAM BANK 1
;*****************************************************************************
mem_map                                 = $a000               ;1024 bytes ($a000-$a3ff)
clipboard_mem                           = $a400               ;3072 bytes ($a400-$afff)

;*****************************************************************************
;CONSTANTS
;*****************************************************************************
.define BNK_SEL $9f61 
.define ROM_SEL $9f60
.define ROM_BNK 7

.define LF 10
.define CR 13          
.define SCREEN_WIDTH 80
.define SCREEN_HEIGHT 54