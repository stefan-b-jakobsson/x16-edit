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
;Function name.......: irq_init
;Purpose.............: Initializes the custom interrupt handler
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc irq_init
    sei
    
    ;Backup default interrupt handler
    lda $0314
    sta irq_default_handler
    lda $0315
    sta irq_default_handler+1

    ;Set vectors to our custom handler
    lda #<irq_handler
    sta $0314
    lda #>irq_handler
    sta $0315

    cli

    rts
.endproc

;******************************************************************************
;Function name.......: irq_handler
;Purpose.............: Custom interrupt handler, effectively the main loop of
;                      the program
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc irq_handler
    ;Check if the program is closing down, i.e. if APP_QUIT!=0
    lda APP_QUIT
    bne shutdown

    ;Main loop
    jsr keyboard_read_and_dispatch
    jsr cursor_toggle
    jsr screen_print_mod_status

    ;Return control to default handler
    jmp (irq_default_handler)

shutdown:
    ;The program is closing, restore irq handler to default
    
    sei

    ;Restore irq handler
    lda irq_default_handler
    sta $0314
    lda irq_default_handler+1
    sta $0315

    ;Hide cursor sprite
    jsr cursor_hide

    ;Clear screen
    lda #147
    jsr KERNAL_CHROUT

    ;Signal to program main process that it's safe to quit
    lda #2
    sta APP_QUIT
    
    cli

    jmp (irq_default_handler)
.endproc

;GLOBAL VARIALBES
irq_default_handler:    .byt 0,0