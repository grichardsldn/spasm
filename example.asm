; generic.asm - generic source file

header:
	goto startup
	nop
	nop
	nop
	goto rtcc_isr  ; interrupts are vectored to address 0004

porta_outputs:
	; routine to set portb to all outputs
	; leaves page at 0

	bsf 3,5 ; use page 1
	clrf 5 ; set PORTB to all outputs
	bcf 3,5 ; use page 0
	return

portb_outputs:
	; routine to set portb to all outputs
	; leaves page at 0

	bsf 3,5 ; use page 1
	clrf 6 ; set PORTB to all outputs
	bcf 3,5 ; use page 0
	return

portb_inputs:
	; routine to set port a to 4 inputs
	; leaves page at 0 
	; upsets w

	bsf 3,5 ; use page 1

	movlw 255 ;
	movwf 6  ; set bottom 4 bits of trisa
	
	bcf 3,5 ; use page 0	
	return

porta_inputs:
	; routine to set port a to 4 inputs
	; leaves page at 0 
	; upsets w

	bsf 3,5 ; use page 1

	movlw 15 ;
	movwf 5  ; set bottom 4 bits of trisa
	
	bcf 3,5 ; use page 0	
	return


enable_interrupts:
	; sets the global interrupt enable bit
	bsf 11, 7 ; set bit 7 of INTCON reg
	return

disable_interrupts:
	; unsets the global interrupt enable bit
	bcf 11,7 ; unset bit 7 of INTCON reg
	return

startup:
	; startup for the board
	; disables interrupts, sets portb to output
	; Enables RT interrupt (for when interrupts enabled)
	; Sets prescalar to RTCC 1:256
	; then calls the labal called 'main'
	
	clrf 6	
	bsf 3,5   ; use page 1

	movlw 7 ; set option to RTCC prescaler 1:256
	movwf 1 ; 

	bcf 3,5	  ; use page 0

	bcf 11, 7	; Set INTCON to Global interrupt enable
	bsf 11, 5	; set RT int enable bit


	bcf 3, 0	; clear the carry bit (god knows why)

	goto main	; run the application	
	

rtcc_isr:
	; This is called when an interrupt happens

	bcf  11, 2 ; clear the interrupt bit
	retfie	; return from interrupt

; ***************************************************************



delay30us:
	; wait a short while
	clrwdt
	nop
	nop
	nop
	nop
	nop
	nop
nop
nop
nop
nop
nop
nop
	;nop
	;nop
	;nop

	return

delay10us:
	; wait a short while
	clrwdt
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop
	;nop

	return

delay1ms:
	; wait 1ms
	; uses address 30 as a counter

	movlw 100
	movwf 30

	d1ms_loop:
		call delay10us
	decfsz 30,f
	goto d1ms_loop ; dec till zero
	return

delay1s:
	; wait 1 second
	; uses address 31 as a counter
	
	movlw 255
	movwf 31

	d18ms_loop0:
		call delay1ms
	decfsz 31, f
	goto d18ms_loop ; dec till zero

	movlw 255
	movwf 31

	d18ms_loop2:
		call delay1ms
	decfsz 31, f
	goto d18ms_loop2 ; dec till zero
	movlw 255
	movwf 31

	d18ms_loop3:
		call delay1ms
	decfsz 31, f
	goto d18ms_loop3 ; dec till zero
	movlw 255
	movwf 31

	d18ms_loop4:
		call delay1ms
	decfsz 31, f
	goto d18ms_loop4 ; dec till zero


	return		

delay18ms:
	; wait 18ms
	; uses address 31 as a counter
	
	movlw 18
	movwf 31

	d18ms_loop:
		call delay1ms
	decfsz 31, f
	goto d18ms_loop ; dec till zero
	return	

output_pulse:
	; output a pulse of *f20 (whats at f20) 10us periods
	; uses address f32

	movf 20, w
	movwf 32  ; set f32 to what was at f20

	bsf 5, 0 ; set bit 0 of porta

	op_loop:
		call delay10us

	decfsz 32, f ; decrement f32
	goto op_loop ; if non zero go round again

	bcf 5, 0 ; unset bit 0 of portb
	return

outputMidi32:
	; output the value of register 32 to 
	; uses register 33 as working space

	; set f33 to f32,  but invert the bits (serial sends 1 is low, 0 is hi)
	movf 32,w
	xorlw 255
	movwf 33


	; write the start bit (a high)
	movlw 1
	movwf 5
	call delay30us


	; write f33 to porta
	movf 33,w	
	movwf 5

	; shift f33 right one
	
	
	rrf 33,f

	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us
	; write f33 to porta
	movf 33,w	
	movwf 5
	; shift f33 right one
	rrf 33,f
	call delay30us

	; write the stop bit (a low)
	movlw 0
	movwf 5
	call delay30us

	return

sendMidiOn:

	; send Midi note on, channel=0, note=100, volume=100
	movlw 144
	movwf 32
	call outputMidi32
	movlw 36
	movwf 32
	call outputMidi32
	movlw 36
	movwf 32
	call outputMidi32
	return

sendMidiOff:

	; send Midi note off, channel=0, note=100, volume=100
	movlw 128
	movwf 32
	call outputMidi32
	movlw 36
	movwf 32
	call outputMidi32
	movlw 36
	movwf 32
	call outputMidi32
	return

	
main:
	; at present dont do anything

	call disable_interrupts
	call porta_outputs
	call portb_inputs

	; address 20 is the variable
	movlw 110
	movwf 20 ; set 20 to 100

	
	; set f32 to 85
	movlw 85
	movwf 32

	main_loop:

		
		call sendMidiOn
		call delay1s
		call sendMidiOff
		call delay1s

		; move port A to f20
		movf 6,w
		movwf 20

		

		; go to the end of this loop
		goto jump

		btfss 5, 1 
		incf 20, f ; if bit 1 of porta is not set inc f20

		btfsc 5, 1 
		decf 20,f ; if set dec f20

		btfss 5, 1
		bcf 6, 7

		btfsc 5, 1
		bsf 6, 7


		; now bounds check - see if its 100
		movlw 100   ;
		subwf 20, w ;
		btfsc 3, 2     ; skip if zero is clear
		incf 20,f      ; if zero was set var = 100, increment it

		; now bounds checks - see if its 200
		movlw 200
		subwf 20, w
		btfsc 3, 2 ; skil is zero is clear
		decf 20, f ; if zero was set var = 200, decrement it

		jump:

		call delay18ms
		goto main_loop

end
	
