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

.import __ZP_START__, __ZP_LAST__, __VARS_LOAD__
.export bridge_code

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

;Keyboard
.define KERNAL_GETIN $ffe4
.define KERNAL_KBDBUF_GET_MODIFIERS $ca64
KBD_MODIFIER_SHIFT = 1 ; C64:  Shift
KBD_MODIFIER_ALT   = 2 ; C64:  Commodore
KBD_MODIFIER_CTRL  = 4 ; C64:  Ctrl
KBD_MODIFIER_WIN   = 8 ; C128: Alt
KBD_MODIFIER_CAPS  = 16; C128: Caps

;Other
.define KERNAL_SCREEN_SET_CHARSET $ff62

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
;ZERO PAGE VECTORS AND VARIABLES
;*****************************************************************************

.segment "ZEROPAGE"

;Pointer to memory location: Cursor position
CRS_BNK:    .res 1   ;Bank
CRS_ADR:    .res 2   ;Address
CRS_IDX:    .res 1   ;Index

;Pointer to memory location: First visible character of the currently edited line
LNV_BNK:    .res 1   ;Bank
LNV_ADR:    .res 2   ;Address
LNV_IDX:    .res 1   ;Byte index

;Pointer to memory location: First visible character of the screen (top left)
SCR_BNK:    .res 1   ;Bank
SCR_ADR:    .res 2   ;Address
SCR_IDX:    .res 1   ;Index

;Temporary pointers to memory locations, used for miscellaneous (short-term) purposes
;Warning: Any routine may use these temp pointers at any time and for whatever reason
TMP1_BNK:   .res 1   ;Bank
TMP1_ADR:   .res 2   ;Address MSB
TMP1_IDX:   .res 1   ;Index

TMP2_BNK:   .res 1   ;Bank
TMP2_ADR:   .res 2   ;Address MSB
TMP2_IDX:   .res 1   ;Index

.CODE

;*****************************************************************************
;Global variables not stored in zero page
;*****************************************************************************

.segment "VARS"

APP_MOD:                .res 1              ;1 byte, application mode
APP_QUIT:               .res 1              ;1 byte, used to signal program exit
tempvars:               .res 16             ;Temporary variables
cmd_display_msg:        .res 59

.CODE

;*****************************************************************************
;Buffers
;*****************************************************************************

.segment "VARS"

prompt_input:           .res 256
file_cur_filename:      .res 256
file_disk_status:       .res 127
bridge_code:            .res 127

.CODE

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