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
;Function name.......: charset_petscii_to_scrcode
;Purpose.............: Macro that converts PETSCII code to screen code
;Input...............: A = PETSCII code
;Returns.............: A = screen code
;Error returns.......: None
;Credits.............: https://sta.c64.org/cbm64pettoscr.html
.macro charset_petscii_to_scrcode
    phx
    tax
    lda screen_scrcodes,x
    plx 
.endmacro

;******************************************************************************
;Function name.......: charset_ascii_to_petscii_ucase
;Purpose.............: Macro that converts ASCII code to PETSCII upper case
;Input...............: A = ASCII value
;Returns.............: A = PETSCII value
;Error returns.......: None
.macro charset_ascii_to_petscii_ucase
    cmp #97
    bcc charset_ascii_to_petscii_ucase_exit

    cmp #123
    bcs charset_ascii_to_petscii_ucase_exit

    sec
    sbc #32

charset_ascii_to_petscii_ucase_exit:
    charset_petscii_to_scrcode
    
.endmacro

;******************************************************************************
;Function name.......: charset_ascii_to_petscii_lcase
;Purpose.............: Macro that converts ASCII code to PETSCII upper/lower 
;                      case
;Input...............: A = ASCII value
;Returns.............: A = PETSCII value
;Error returns.......: None
.macro charset_ascii_to_petscii_lcase
    cmp #65
    bcc charset_ascii_to_petscii_lcase_exit

    cmp #91
    bcc charset_ascii_to_petscii_lcase_add32

    cmp #97
    bcc charset_ascii_to_petscii_lcase_exit

    cmp #123
    bcc charset_ascii_to_petscii_lcase_sub32

    jmp charset_ascii_to_petscii_lcase_exit

charset_ascii_to_petscii_lcase_add32:
    clc
    adc #32
    jmp charset_ascii_to_petscii_lcase_exit

charset_ascii_to_petscii_lcase_sub32:
    sec
    sbc #32

charset_ascii_to_petscii_lcase_exit:
    charset_petscii_to_scrcode

.endmacro