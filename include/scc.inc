;scc.inc


_sccOUT:
;set to portB
	SEX R2

	DEC R2
	LDI 0x01
	STR R2
	out 1
	
	DEC R2
	LDI 0x01
	STR R2
	out 4
	;due to now the SCC address lines being tied to IO_A0,IO_A0,FLASH_CE we use this
	
	;DEC R2
$$checkoutready:

;	inp 6
;	ani 0x0C ;0000 1100 ;mask so other bits dont mess this
;	xri 0x0C ;xor immidiate, so test, if both bits are 1 then we get 0, else 1
;	bnz $$checkoutready ;loop untill ready

	inp 6
	ani 0x0C ;0000 1100 ;mask so other bits dont mess this
	xri 0x0C ;xor immidiate, so test, if both bits are 1 then we get 0, else 1
	lbnz $$checkoutready ;loop untill ready


	;seq
	DEC R2
	LDI 0x03
	STR R2
	out 4;due to now the SCC address lines being tied to IO_A0,IO_A0,FLASH_CE we use this
	;out 1
	
	DEC 2
	glo R12
	str R2
	out 6
	;req
	cretn





_sccIN:
;set to portB
	sex R2

	DEC R2
	LDI 0x01
	STR R2
	out 1
	
	DEC sp
	LDI 0x01
	STR sp
	out 4
	
	inp 6
	ani 0x01
	LBZ _not_CTS00 ;if there is still stuff on the buffer. skip the CTS
	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4
	
	
	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4
    ;----------------
	
	;since this is negated. we basicly pull OP[0] low, so CTS allows stuff to be sent (Assumes MR1A[7]=1)
	DEC sp
	LDI 0x01
	STR sp
	out 6
	;----------------

	DEC R2
	LDI 0x01
	STR R2
	out 4
_not_CTS00:
$$checkinpready:

	inp 6
	ani 0x01
	lbz $$checkinpready ;loop untill ready

	DEC R2
	LDI 0x03
	STR R2
	out 4
	;out 1
	

	INP 6

	;DEC 2
	plo R15
	cretn


_gets_asm:
	;asm version of gets()
	DEC sp
	LDI 0x01
	STR sp
	out 1

__getsloop:


	DEC sp
	LDI 0x01
	STR sp
	out 4
	
	inp 6
	ani 0x01
	LBZ _not_CTS01 ;if there is still stuff on the buffer. skip the CTS
	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4
    ;----------------
	
	;since this is negated. we basicly pull OP[0] low, so CTS allows stuff to be sent (Assumes MR1A[7]=1)
	DEC sp
	LDI 0x01
	STR sp
	out 6
	;----------------

	DEC sp
	LDI 0x01
	STR sp
	out 4
_not_CTS01:
$$checkinpready45:
	inp 6
	ani 0x01
	lbz $$checkinpready45 ;loop untill ready
	DEC sp
	LDI 0x03
	STR sp
	out 4
	INP 6 ;fetch char
	STR R12 ;store char thru pointer
	;now the fun stuff to printback
	SEX R12 ;please dont break
	OUT 6 ;also this increments the X reg too, so R12 gets incremented
	SEX sp ;make R2 the X reg again
	;INC R12
	PLO R13;store in R13 the received char temporarelly
	XRI 0x08 ;check if its a backspace
	LBNZ $$notbackspace
	;if it is
	DEC R12;go back to the char we just stored(backspace)
	LDI 0x00 ;put a 0 there
	DEC R12 ;dec again
	LBR __getsloop ;run loop again
	
$$notbackspace:
	GLO R13
	XRI '\r' ;xor it with \r
	LBNZ __getsloop ;if not equal we go back
	DEC R12
	LDI 0x00
	STR R12 ;remove \r and assure a 0x00 at end of string
	;else return
	Cretn
	

_getsSized_asm:
	;temp fix------
	;RLDI R12,0x8000
	;--------------
	
	;RSXD R12;store our R12
	GLO R12
	STXD 
	GHI R12
	STXD
	
	DEC sp
	LDI 0x01
	STR sp
	out 1
__getsSizedloop:

	DEC sp
	LDI 0x01
	STR sp
	out 4
	
	inp 6
	ani 0x01
	LBZ _not_CTS02 ;if there is still stuff on the buffer. skip the CTS
	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4


	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4
    ;----------------
	
	;since this is negated. we basicly pull OP[0] low, so CTS allows stuff to be sent (Assumes MR1A[7]=1)
	DEC sp
	LDI 0x01
	STR sp
	out 6
	;----------------


	DEC sp
	LDI 0x01
	STR sp
	out 4
_not_CTS02:
$$checkinpready98:
	inp 6
	ani 0x01
	lbz $$checkinpready98 ;loop untill ready
	DEC sp
	LDI 0x03
	STR sp
	out 4
	INP 6 ;fetch char
	STR R12 ;store char thru pointer
	INC R12
	;DBNZ R13,__getsSizedloop ;loop if we havent gone thru the n times
	DEC R13
	GHI R13
	LBNZ __getsSizedloop
	GLO R13
	LBNZ __getsSizedloop
	;RLXA R12 ;get OLD R12
	LDXA
	PHI R12
	LDXA
	PLO R12
	Cretn

_getsRTS_asm:
	;fetches data while RTS is on (RTS=0)
	;temp fix------
	;RLDI R12,0x8000
	;--------------

	LDI 0x00
	PHI R15
	LDI 0x00
	PLO R15
	;now returns the size of the load
		
	;RSXD R12;store our R12
	GLO R12
	STXD 
	GHI R12
	STXD
	
	DEC sp
	LDI 0x01
	STR sp
	out 1
__getsRTSloop:

	DEC sp
	LDI 0x01
	STR sp
	out 4
	
	inp 6
	ani 0x01
	LBZ _not_CTS043 ;if there is still stuff on the buffer. skip the CTS
	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4


	;go to the OP REG
	DEC sp
	LDI 0x0E
	STR sp
	out 4
    ;----------------
	
	;since this is negated. we basicly pull OP[0] low, so CTS allows stuff to be sent (Assumes MR1A[7]=1)
	DEC sp
	LDI 0x01
	STR sp
	out 6
	;----------------


	DEC sp
	LDI 0x01
	STR sp
	out 4
_not_CTS043:
$$checkinpready983:
	inp 6
	ani 0x01
	lbz $$checkinpready983 ;loop untill ready
	DEC sp
	LDI 0x03
	STR sp
	out 4
	INP 6 ;fetch char
	STR R12 ;store char thru pointer
	INC R12
	INC R15
	;DBNZ R13,__getsSizedloop ;loop if we havent gone thru the n times
	
	DEC sp
	LDI 0x0D ;get Inputs
	STR sp
	out 4
	INP 6
	ANI 0x01

	LBZ __getsRTSloop
	
	;RLXA R12 ;get OLD R12
	LDXA
	PHI R12
	LDXA
	PLO R12
	Cretn


; _getbuffer:

; $$getbufferloop:

	; Ccall _sccIN;get char
	; GLO R15
	; STR R12
	; INC R12
	
	; SEX R13 ;get EOF char at this register
	; AND ;AND it
	; BZ $$getbufferloop
	; Cretn


_getINTregister:

	SEX R2

	DEC R2
	LDI 0x01
	STR R2 ;put 0x01 on IO 1 to go to port B
	out 1
	
	DEC R2
	LDI 0x05 ;go to Interrupt Status Register (ISR)
	STR R2
	out 4
	
	inp 6 ;read the register
	plo R15;return it
	cretn
	
_setINTregister

	SEX R2

	DEC R2
	LDI 0x01
	STR R2 ;put 0x01 on IO 1 to go to port B
	out 1
	
	DEC R2
	LDI 0x05 ;go to Interrupt Mask Register (IMR)
	STR R2
	out 4
	
	DEC 2
	glo R12 ;get the value we want to put in IMR
	str R2
	out 6 ;send it
	cretn
	
	
	
;Ccall "function"



;for TX and RX channel 2

_sccIN2:
;set to portB
	sex R2

	DEC R2
	LDI 0x01
	STR R2
	out 1

	DEC R2
	LDI 0x09
	STR R2
	out 4

$$checkinpready2:

	inp 6
	ani 0x01
	lbz $$checkinpready2 ;loop untill ready

	DEC R2
	LDI 0x11
	STR R2
	out 4
	;out 1
	

	INP 6

	;DEC 2
	plo R15
	cretn

_sccOUT2:
;set to portB
	SEX R2

	DEC R2
	LDI 0x01
	STR R2
	out 1
	
	DEC R2
	LDI 0x09
	STR R2
	out 4
	;due to now the SCC address lines being tied to IO_A0,IO_A0,FLASH_CE we use this
	
	;DEC R2
$$checkoutready2:

;	inp 6
;	ani 0x0C ;0000 1100 ;mask so other bits dont mess this
;	xri 0x0C ;xor immidiate, so test, if both bits are 1 then we get 0, else 1
;	bnz $$checkoutready ;loop untill ready

	inp 6
	ani 0x0C ;0000 1100 ;mask so other bits dont mess this
	xri 0x0C ;xor immidiate, so test, if both bits are 1 then we get 0, else 1
	lbnz $$checkoutready2 ;loop untill ready


	;seq
	DEC R2
	LDI 0x0B
	STR R2
	out 4;due to now the SCC address lines being tied to IO_A0,IO_A0,FLASH_CE we use this
	;out 1
	
	DEC 2
	glo R12
	str R2
	out 6
	;req
	cretn
