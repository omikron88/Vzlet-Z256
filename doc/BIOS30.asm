;
;                ***********************************
;                *                                 *
;                *        Z256 Microcomputer       *
;                *                                 *
;                ***********************************
;                *                                 *
;                *   EPROM Procedures Ver 3.0      *
;                *                                 *
;                *  (C) 1988 by Honza Petr Jirka   *
;                *                                 *
;                *     Last UpDate: 27.09.1988     *
;                *                                 *
;                ***********************************
;
;*****************************************************************
;*                                                               *
;*                   EQU DEFINITIONS                             *
;*                                                               *
;*****************************************************************
;       Version and Date of generated system
MAINVR	EQU	'3'		;main version number
SUBVER	EQU	'00'		;sub version number
;
DAY	EQU	'27'		;day
MONTH	EQU	'09'		;month
YEAR	EQU	'88'		;year of generating
;
;*****************************************************************
;*                                                               *
;*                  EPROM BIOS JUMP'S Table                      *
;*                                                               *
;*****************************************************************
;
	cpu	z80
;
	ORG	0000H		;begin of EPROM BIOS
;                             
	JP	RESET		;system start up procedure & CP/M load
	JP	RDSYS		;read CP/M from floppy
;
	JP	CONST		;console status
	JP	CONIN		;console input
;
	JP	CONOUT		;console output
	JP	STROUT		;string output on console
;
	JP	RESTOR		;restore driver to track 00
	JP	SEEKTR		;seek head to desired track
	JP	FREAD		;read one track from floppy
	JP	FWRITE		;write one track to floppy
	JP	SDRIVE		;select drive
	JP	FLOPIO		;direct input/output to floppy
	JP	RDTRK		;read entire track from floppy
	JP	FRMTRK		;format track
	JP	RDADR		;read address floppy head
;
	JP	MWINDW		;move window to/from data block
	JP	RVIDEO		;read bytes from video Board
	JP	WVIDEO		;write bytes to Video Board
	JP	SETVMD		;set video mode
	JP	PLOT		;write one dot in graphic mode
	JP	DRAW		;draw line in graphic mode
	JP	SETPEN		;set pen for Plot and Draw
	JP	EXPFNT		;expand font to font table
;
	JP	PINIT 		;port('s) initialization procedure
	JP	SS1B		;serial status channel 1B
	JP	SI1B		;serial input channel 1B
	JP	SO1B		;serial output channel 1B
	JP	PS1		;parallel status channel 1
	JP	PO1		;parallel output channel 1
;
;-----------------------------------------------------------------------------
;
	DEFB	'(c) Kotek'	;Copyright (c) 1988 by P. Kotek
;
;	Version number
;
VERSNO:	DEFB	MAINVR,'.',SUBVER
;
;	System configuration default value
;
SYSCNF:	DEFB	0,0
;
;-----------------------------------------------------------------------------
;
;	Non Mascable Interrupt Entry Point
;
NMIENT:	JP	NMISRV	;jump to NMI service subroutine
;
;=============================================================================
;
;	Include Input/Output devices address
;
;         ***********************************
;         *                                 *
;         *       Z256 Microcomputer        *
;         *                                 *
;         ***********************************
;         *                                 *
;         *   I/O Ports Address Ver 3.0     *
;         *                                 *
;         *   Copyright (C) 1988 by Petr    *
;         *                                 *
;         *     Last UpDate: 16.07.1988     *
;         *                                 *
;         ***********************************
;
;
;********************************************************
;*                                                      *
;*            CPU Board I/O Ports Address               *
;*                                                      *
;********************************************************
;
;                    DMA (IO6)
;
;-----------------------------------------------------------------------------
;
DMA1SC	EQU	0ECH	;Status/Control register of DMA
;
;=============================================================================
;
;                    PIO (IO8)
;
;-----------------------------------------------------------------------------
;
PIO1AD	EQU	0F0H	;PIO channel A Data register
PIO1BD	EQU	0F1H	;PIO channel B Data register
PIO1AC	EQU	0F2H	;PIO channel A Control register
PIO1BC	EQU	0F3H	;PIO channel B Control register
;
;=============================================================================
;
;                    CTC (IO7)
;
;-----------------------------------------------------------------------------
;
CTC10	EQU	0F4H	;CTC channel 0
CTC11	EQU	0F5H	;CTC channel 1
CTC12	EQU	0F6H	;CTC channel 2
CTC13	EQU	0F7H	;CTC channel 3
;
;=============================================================================
;
;                    SIO (IO23)
;
;-----------------------------------------------------------------------------
;
SIO1AD EQU	0F8H	;SIO channel A Data register
SIO1BD EQU	0F9H	;SIO chanell B Data register
SIO1AC EQU	0FAH	;SIO chanell A Control register
SIO1BC EQU	0FBH	;SIO chanell B Control register
;
;=============================================================================
;
;                    Bank switch register (IO21,IO22)
;
;-----------------------------------------------------------------------------
;
BANK	EQU	0FCH	;Bank switch register for switch
			;64kbyte page of memory
;
;      Control word for banking
;
; Bits 1,0 - page address in read memory cycle
; Bits 3,2 - page address in write memory cycle
; Bits 5,4 - page address in OP code fetch cycle
; Bit  6	 - unused
; Bit  7   - EPROM & RAM in CPU Board switch
;            (log.0 - EPROM & RAM On)
;
;
;
;*********************************************************
;*                                                       *
;*            RAM Board I/O Ports Address                *
;*                                                       *
;*********************************************************
;
;          Memory board switch register (IO11)
;
;-----------------------------------------------------------------------------
;
MBOARD	EQU	0C0H	;Output to this port enables
			;Master RAM board for access
SBOARD	EQU	0C1H	;Output to this port enables
			;Slave board (CRT) for access
MBRDAP	EQU	0C2H	;Output to this port enables
			;Master RAM board for access
			;with parity check of data
;
;************************************************************
;*                                                          *
;*           FTC Board I/O Ports Address                    *
;*                                                          *
;************************************************************
;
;                    FDC (IO6)
;
;-----------------------------------------------------------------------------
;
FDCSCR	EQU	0D0H	;FDC Status/Command Register
FDCTRK	EQU	0D1H	;FDC Track register
FDCSEC	EQU	0D2H	;FDC Sector register
FDCDAT	EQU	0D3H	;FDC Data register
;
;-----------------------------------------------------------------------------
;
;                    PIO (IO4)
;
;-----------------------------------------------------------------------------
;
PIO2AD	EQU	0D4H	;PIO channel A Data register
PIO2BD	EQU	0D5H	;PIO channel B Data register
PIO2AC	EQU	0D6H	;PIO channel A Control register
PIO2BC	EQU	0D7H	;PIO channel B Control register
;
;=============================================================================
;
;                    CTC (IO24)
;
;-----------------------------------------------------------------------------
;
CTC20	EQU	0D8H	;CTC channel 0
CTC21	EQU	0D9H	;CTC channel 1
CTC22	EQU	0DAH	;CTC channel 2
CTC23	EQU	0DBH	;CTC channel 3
;
;=============================================================================
;
;                    SIO (IO27)
;
;-----------------------------------------------------------------------------
;
SIO2AD	EQU	0DCH	;SIO channel A Data register
SIO2AC	EQU	0DDH	;SI0 channel A Control register
SIO2BD	EQU	0DEH	;SIO channel B Data register
SIO2BC	EQU	0DFH	;SIO channel B Control register
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
;	Include Input/Output devices commands
;
;         ***********************************
;         *                                 *
;         *       Z256 Microcomputer        *
;         *                                 *
;         ***********************************
;         *                                 *
;         * Commands for I/O dev.  Ver 3.0  *
;         *                                 *
;         *   Copyright (C) 1988 by Petr    *
;         *                                 *
;         *     Last UpDate: 24.09.1988     *
;         *                                 *
;         ***********************************
;
;********************************************************
;*                                                      *
;*                 PIO Commands                         *
;*                                                      *
;********************************************************
;
PIOOUT	EQU	00FH	;set channel to Output (mode 0)
PIOIN	EQU	04FH	;set channel to Input (mode 1)
PIOBID	EQU	08FH	;set channel to bidirectional (mode 2)
PIOCTR	EQU	0CFH	;set channel to control (mode 3)
			;In control mode the next word must set
			;the I/O Register. In this word log.1 set
			;proper bit as input and log.0 as output
PIOIEN EQU	083H	;enable interrupt from channel
PIOIDI EQU	003H	;disable interrupt from channel
;
;Next interrupt control word is used in mode 3 only and must be followed
;by a mask. Only if mask bit is zero then will be monitored for 
;generating an interrupt.
;
PIOIOL	EQU	097H	;en. int., OR condition on Low level
PIOIOH	EQU	0B7H	;en. int., OR condition on High level
PIOIAL	EQU	0D7H	;en. int., AND condition on Low level
PIOIAH	EQU	0F7H	;en. int., AND condition on High level
;
;
;********************************************************
;*                                                      *
;*                 CTC Commands                         *
;*                                                      *
;********************************************************
;
CTCRES	EQU	003H	;dis.int.,Timer mode,Prescaler 16,neg.edge,
			;timer starts after loading time constant,
			;No time constant follow,Stop channel.
; Next control words may be ORed with CTCRES command.
;
CTCIEN	EQU	080H	;Enable Interrupt from channel
CTCCNT	EQU	040H	;set Counter mode
CTC256	EQU	020H	;set Prescaler to 256
CTCPEG	EQU	010H	;set positive edge for timer or counter mode
CTCTEX	EQU	008H	;external trigger is valid for starting timer
CTCLTC	EQU	004H	;next word is loaded in to Time Constant Reg.
;
;
;********************************************************
;*                                                      *
;*                 SIO Commands                         *
;*                                                      *
;********************************************************
;
;	SIO registers pointers
;
; SIO reg. pointers may be "OR"ed with SIO Commands and
;                                      SIO CRC Reset Code
;
SIOR0	EQU	00	;pointer to register 0
SIOR1	EQU	01	;pointer to register 1
SIOR2	EQU	02	;pointer to register 2
SIOR3	EQU	03	;pointer to register 3
SIOR4	EQU	04	;pointer to register 4
SIOR5	EQU	05	;pointer to register 5
SIOR6	EQU	06	;pointer to register 6
SIOR7	EQU	07	;pointer to register 7
;
;-----------------------------------------------------------------------------
;
;	SIO commands
;
SIOSAB	EQU	00001000B	;Send Abort (SDLC Mode)
SIOREI	EQU	00010000B	;Reset External/Status Interrupt
SIORES	EQU	00011000B	;Chanell Reset
SIOINR	EQU	00100000B	;Enable Interrupt on Next
			;Rx Character
SIORTI	EQU	00101000B	;Reset Transmitter Interrupt
			;Pending
SIOERS	EQU	00110000B	;Error Reset (latches)
SIORFI	EQU	00111000B	;Return From Interrupt (Channel A)
;
;-----------------------------------------------------------------------------
;	SIO CRC Reset Code
;
SIORRC	EQU	01000000B	;Reset Receive CRC Checker
SIORTG	EQU	10000000B	;Reset Transmit CRC Generator
SIORTU	EQU	11000000B	;Reset Tx Underrun/End Of Message latch
;
;-----------------------------------------------------------------------------
;
;	Register 1 Controls Bits
;
SIOESI	EQU	00000001B	;External/Status Interrupt Enable
SIOTIE	EQU	00000010B	;Transmitter Interrupt Enable
SIOSAV	EQU	00000100B	;Status Affect Vector
;
;	Receive Interrupts
;
SIORIF	EQU	00001000B	;Receive Int. On First Char. Only
SIOIAP	EQU	00010000B	;Int. on All Rec. Chars.-
			;Parity error is a Special Rec. Condition
SIOIOA	EQU	00011000B	;Int. On All Rec. Chars.-
			;Parity error is not a Spec.Rec.Condition
;
SIOWRR	EQU	00100000B	;WAIT/READY Output respond to the condition
			;on Receive Buffer
SIORFN	EQU	01000000B	;select Ready Function
SIOWRE	EQU	10000000B	;Wait/Ready Function Enable
;
;-----------------------------------------------------------------------------
;
;	Register 2 Interrupt Vector Register
;
;-----------------------------------------------------------------------------
;
;	Register 3 Receiver Control Bits and Parameters
;
SIOREE	EQU	00000001B	;Receiver Enable
SIOSCI	EQU	00000010B	;Sync Character Load Inhibit
SIOASM	EQU	00000100B	;Address Search Mode
SIORCE	EQU	00001000B	;Receiver CRC Enable
SIOEHP	EQU	00010000B	;Enter Hunt Phase
SIOAUE	EQU	00100000B	;Auto Enables
;
SIORC5	EQU	00000000B	;5 Receiver Bits/Character
SIORC6	EQU	10000000B	;6 Receiver Bits/Character
SIORC7	EQU	01000000B	;7 Receiver Bits/Character
SIORC8	EQU	11000000B	;8 Receiver Bits/Character
;
;-----------------------------------------------------------------------------
;
;	Register 4 Receiver and Transmitter Control Bits
;
SIOPAE	EQU	00000001B	;Parity enable
SIOPEV	EQU	00000010B	;Parity Even
;
SIOSYN	EQU	00000000B	;Synchronous mode
SIOSB1	EQU	00000100B	;1 Stop Bit per character
SIOS1H	EQU	00001000B	;1 1/2 Stop Bit per character
SIOSB2	EQU	00001100B	;2 Stop Bits per character
;
;	Sync Modes
;
SIOSYB	EQU	00000000B	;8 bit programed sync
SIOS16	EQU	00010000B	;16 bit programed sync
SIOSDL	EQU	00100000B	;SDLC Mode
SIOESM	EQU	00110000B	;External Sync Mode
;
;	Clock Rate
;
SIOC01	EQU	00000000B	;DataRate=1*ClockRate
SIOC16	EQU	01000000B	;DataRate=16*ClockRate
SIOC32	EQU	10000000B	;DataRate=32*ClockRate
SIOC64	EQU	11000000B	;DataRate=64*ClockRate
;
;-----------------------------------------------------------------------------
;
;	Register 5 Transmitter Control Bits
;
SIOTCE	EQU	00000001B	;Transmit CRC Enable
SIORTS	EQU	00000010B	;Neg. Output RTS Set to 0
				;Request To Send
SIOR16	EQU	00000100B	;Set CRC-16 polynomial
SIOTRE	EQU	00001000B	;Transmit Enable
SIOSBR	EQU	00010000B	;Send Break
;
;	Transmit Characters
;
SIOTR5	EQU	00000000B	;Transmit 5 Bits/Character
SIOTR6	EQU	01000000B	;Transmit 6 Bits/Character
SIOTR7	EQU	00100000B	;Transmit 7 Bits/Character
SIOTR8	EQU	01100000B	;Transmit 8 Bits/Character
;
SIODTR	EQU	10000000B	;Neg. Output DTR Set to 0
				;Data Terminal Ready
;
;-----------------------------------------------------------------------------
;
;
;********************************************************
;*                                                      *
;*                 Bank Register Commands               *
;*                                                      *
;********************************************************
;
; Commands for switch EPROM & RAM on CPU Board. It may be
; ORed with commands for page setting.
;
EPRON	EQU	000H	;set EPROM & RAM and all page to 0
EPROFF	EQU	080H	;set EPROM & RAM Off and all page to 0
;
;	Page setting commands
;
RDMP0	EQU	000H	;set memory page 0 in Read Memory cycle
RDMP1	EQU	001H	;set memory page 1 in Read Memory cycle
RDMP2	EQU	002H	;set memory page 2 in Read Memory cycle
RDMP3	EQU	003H	;set memory page 3 in Read Memory cycle
;
WRMP0	EQU	000H	;set memory page 0 in Write Memory cycle
WRMP1	EQU	004H	;set memory page 1 in Write Memory cycle
WRMP2	EQU	008H	;set memory page 2 in Write Memory cycle
WRMP3	EQU	00CH	;set memory page 3 in Write Memory cycle
;
M1MP0	EQU	000H	;set memory page 0 in OP code fetch cycle
M1MP1	EQU	010H	;set memory page 1 in OP code fetch cycle
M1MP2	EQU	020H	;set memory page 2 in OP code fetch cycle
M1MP3	EQU	030H	;set memory page 3 in OP code fetch cycle
;
;
;********************************************************
;*                                                      *
;*                 FDC Commands for WD2797              *
;*                                                      *
;********************************************************
;
FRESTR	EQU	000H	;FDC Restore (seek to track 00)
FSEEK	EQU	010H	;FDC Seek (Track Reg.=current position
			;          Data  Reg.=desired track no.)
FSTEP	EQU	020H	;One step in previous direction
FSTIN	EQU	040H	;One step towards track 76
FSTOUT	EQU	060H	;One step towards track 00
;
; Commands FRESTR, FSEEK, FSTEP, FSTIN and FSTOUT may be ORed
; with followed flags.
;
FSRT3	EQU	000H	;Step Rate 3ms  (8") or 6ms  (5")
FSRT6	EQU	001H	;Step Rate 6ms  (8") or 12ms (5")
FSRT10	EQU	002H	;Step Rate 10ms (8") or 20ms (5")
FSRT15	EQU	003H	;Step Rate 15ms (8") or 30ms (5")
;
FTRVRF	EQU	004H	;verify on desired track
FHEADL	EQU	008H	;load head at beginning of command
;
; Commands FSTEP, FSTIN and FSTOUT may be ORed with
; followed flag.
;
FTRKUP	EQU	010H	;update track register
;
; Commands for Read/Write data on to disk
;
FRDSEC	EQU	088H	;read one sector from disk
FWRSEC	EQU	0A8H	;write one sector on disk
FRDADR	EQU	0C0H	;read address from ID field
FRDTRK	EQU	0E0H	;read one track from disk
FWRTRK	EQU	0F0H	;write one track on disk (formating)
;
; Commands FRDSEC, FWRSEC, FRDADR, FRDTRK and FWRTRK may be ORed
; with followed flags.
;
FSIDE1	EQU	002H	;update Side Select Output to log.0
FDELAY	EQU	004H	;Head loaded testing after 15ms (8")
			; or 30ms (5")
;
; Commands FRDSEC and FWRSEC may be ORed with followed flag.
;
FMSEC	EQU	010H	;Read or Write multiple sectors.
;
; Commands for terminating any FDC commands and interrupts
;
FABORT	EQU	0D0H	;abort any command presently under
			;execution without interrupt
;
; FABORT may be ORed with followed flags to generating
; conditional interrupts.
;
FNRRI	EQU	001H	;interrupt if Not-Ready to Ready Transition
FRNRI	EQU	002H	;interrupt if Ready to Not-Ready Transition
FEIPI	EQU	004H	;interrupt on Every Index Pulse
FIMMI	EQU	008H	;Immediate Interrupt (must be cleared with
			;only FABORT command).
;
;-----------------------------------------------------------------------------
;
;       Floppy disks mask - DRVMSK
;
DRVR0	EQU	00000000B	;driver 0 mask
DRVR1	EQU	00000001B	;driver 1 mask
DRVR2	EQU	00000010B	;driver 2 mask
DRVR3	EQU	00000011B	;driver 3 mask
;
SDENS	EQU	00000100B	;Single Density
DDENS	EQU	00000000B	;Double Density
HDENS	EQU	00101000B	;High Density on Teac GFV
;
EIGHT	EQU	00001000B	;8" driver
FIVE	EQU	00000000B	;5 1/4" driver
;
MOTON	EQU	00010000B	;signal Motor On set to active state
;
INUSE	EQU	00100000B	;signal In Use set to active state
;
MOTSW	EQU	10000000B	;perform motor switch On/Off with delay
;
;-----------------------------------------------------------------------------
;
;       Floppy disks characteristic - DRVCHR
;
TWOSID	EQU	10000000B	;two side driver
;
DBSTP	EQU	01000000B	;double steps
;
FLOPPY	EQU	00000000B	;floppy disk type
HARD	EQU	00010000B	;hard disk type
RAMDSK	EQU	00100000B	;RAM disk type
ROMDSK	EQU	00110000B	;ROM disk type
;
ISTPCL	EQU	00001000B	;invert FDC clock for stepping
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
;	Include System Configuration Constants
;
;
;                ***********************************
;                *                                 *
;                *        Z256 Microcomputer       *
;                *                                 *
;                ***********************************
;                *                                 *
;                *  System Config. Const. Ver 3.0  *
;                *                                 *
;                *   Copyright (C) 1988 by Petr    *
;                *                                 *
;                *     Last UpDate: 24.09.1988     *
;                *                                 *
;                ***********************************
;
;
;****************************************************************
;*                                                              *
;*                  CP/M constants and addresses                *
;*                                                              *
;****************************************************************
;
MSIZE	EQU	64		;size of operating RAM memory
BIAS	EQU	(MSIZE-20)*1024 ;offset to 20k byte CP/M
CCP	EQU	3400H+BIAS	;begin of CCP
CPMLEN	EQU	1600H		;length of CCP and BDOS
BDOS	EQU	CCP+806H	;BDOS entry point
BIOS	EQU	CCP+CPMLEN	;begin of BIOS (BOOT routine)
SECLEN	EQU	0080H		;length of sector on floppy disk
DPBOFF	EQU	10		;DPB address offset in DPH table
DCHADD	EQU	VWRKAR + 18	;end of driver characteristics table in 2114 
RETRY	EQU	3		;number of retries in floppy disk I/O
KBUFLN	EQU	16		;length of keyboard buffer (must be power 2)
;
;****************************************************************
;*                                                              *
;*                  CP/M Default settings                       *
;*                                                              *
;****************************************************************
;
NMISRV	EQU	BIOS+3*18	;default address of NMI service procedure
;
IVTAB	EQU	0FF80H		;default address of interrupt vectors table
;
CTC1VT	EQU	IVTAB		;addr. of CTC 1 int. service procedures
PIO2BV	EQU	IVTAB+8		;addr. of PIO 2 ch. B int. service procedure
PIO2AV	EQU	IVTAB+10	;addr. of PIO 2 ch. A int. service procedure
;
;****************************************************************
;*                                                              *
;*                  System configuration constants              *
;*                                                              *
;****************************************************************
;
SYSCLK	EQU	4		;system clock in [MHz]
;
WEBIOS	EQU	(0-192)&65535	;Begin of EPROM BIOS work Area
BIOSSP	EQU	0		;BIOS Stack Pointer
;
PERD1	EQU	10		;length of CTC12 periode in mili sec.
;
S1BBR	EQU	9600		;Serial channel 1 B baud rate
;
DUMMY	EQU	0FFFFH		;pattern for EPROM reprogramming
;
TEMPSP	EQU	VWRKAR		;temporary stack for floppy procedures
;
LOADER	EQU	0F000H		;begin of Loader area
;
;****************************************************************
;*                                                              *
;*                  EPROM Procedures Entry Points               *
;*                                                              *
;****************************************************************
;
ERESET	EQU	3*00		;System start up procedure & CP/M load
ERDSYS	EQU	3*01		;read CP/M from floppy
;
ECONST	EQU	3*02		;console status (in ver. 3.0 not impl.)
ECONIN	EQU	3*03		;console input (in ver. 3.0 not impl.)
ECONOU	EQU	3*04		;console output
ESTROU	EQU	3*05		;string output on console
;
ERESTR	EQU	3*06		;restore driver to track 00
ESEEKT	EQU	3*07		;seek head to desired track
EFREAD	EQU	3*08		;read one track from floppy
EFWRIT	EQU	3*09		;write one track to floppy
ESDRVR	EQU	3*10		;select drive
EFLPIO	EQU	3*11		;direct input/output to floppy
ERDTRK	EQU	3*12		;read entire track from floppy
EFRMTR	EQU	3*13		;format track
ERDADR	EQU	3*14		;read address floppy head
;
EMWINW	EQU	3*15		;move window to/from data block
ERDVID	EQU	3*16		;read bytes from Video Board
EWRVID	EQU	3*17		;write bytes to Video Board
ESETVM	EQU	3*18		;set video mode
EPLOT	EQU	3*19		;write one dot in graphic mode
EDRAW	EQU	3*20		;draw line in graphic mode
ESTPEN	EQU	3*21		;set pen for Plot and Draw
EEXPFN	EQU	3*22		;expand font to font table
;
EPINIT	EQU	3*23		;port('s) initialization procedure
ESS1B	EQU	3*24		;serial status channel 1B
ESI1B	EQU	3*25		;serial input channel 1B
ESO1B	EQU	3*26		;serial output channel 1B
EPS1	EQU	3*27		;parallel status channel 1
EPO1	EQU	3*28		;parallel output channel 1
;
;****************************************************************
;*                                                              *
;*                  FLOPPY EQU DEFINITIONS                      *
;*                                                              *
;****************************************************************
;
WORKA3	EQU	0FF00H	;work area in page 3 for FDC service
			;subroutines
;
NBINI	EQU	0A2H	;next byte of INI instruction
NBOUTI	EQU	0A3H	;next byte of OUTI instruction
;
; ----------------------------------------------------------------------------
;
;	FFLAGS - floppy flags
;
;	Bit 7                                                   bit 0
;	+------------------------------------------------------------+
;	|  -  |  -  |  -  | MustSv | TrkInB | MustWr | WrSec | WrDir |
;	+------------------------------------------------------------+
;
WRDIR	EQU	0		;set if write sector to directory
WRSEC	EQU	1		;set if write sector
MUSTWR	EQU	2		;set if track must be written to disk
TRKINB	EQU	3		;set if track is loaded in buffer
MUSTSV	EQU	4		;set if track no. or disk is changed
				;and old track must be saved to disk
WRSECM	EQU	00000010B	;write sector flag mask
WSWDMS	EQU	00011100B	;WRSEC and WRDIR clear mask
;
;****************************************************************
;*                                                              *
;*                  CRT CONSTANTS AND COMMANDS                  *
;*                                                              *
;****************************************************************
;
COPBUF	EQU	2000H		;begin of copy buffer
CRTUST	EQU	2000H		;begin of user Motorola registers table
VWRKAR	EQU	2200H		;begin of video work area
CBUFLN	EQU	512		;length of copy buffer
;
;SYSFNT	EQU	4000H		;begin of system font
CHGEPR	EQU	4000H		;begin of Character Generator EPROM
;EXBLCK	EQU	512+16		;length of External Data and Procedures Block
;
UNDLNS	EQU	7F00H		;area for save underline bytes
;
;-----------------------------------------------------------------------------
;
XMAX	EQU	80		;max number of columns
YMAX	EQU	24		;max number of lines
YPIXEL	EQU	12		;number of lines per char
UNDLIN	EQU	10		;line for underlining
MAXFNT	EQU	5		;max number of fonts in RAM
MOTRGS	EQU	12		;number of initialised Motorola registers
;
;-----------------------------------------------------------------------------
;       Supported Control Codes
;
BSPACE	EQU	08H		;shift cursor left one position
				;without deletion a character
LF	EQU	0AH		;Line feed
CR	EQU	0DH		;Carriage return
CLRSVM	EQU	1AH		;Clear screen command (^Z)
ESC	EQU	1BH		;code of escape command
CRLF	EQU	1FH		;Carriage return and Line feed (^_)
SPACE	EQU	20H		;code of space char
DEL	EQU	7FH		;delete one char. left from cursor
;
;-----------------------------------------------------------------------------

;       Supported Escape sequences
;
CLREOL	EQU	'T'		;ESC T Clear to End of Line
;
PALLCH	EQU	92		;'\'print all next character (control)
PONECH	EQU	'^'		;print one next character (control)
;
GOXY	EQU	'='		;Goto XY lead in character (ESC = YX)
;
SETUND	EQU	'U'		;set underline command
RESUND	EQU	'N'		;reset underline command
;
INVCOL	EQU	'I'		;set invert fore and background color
NORMCL	EQU	'O'		;set original fore and background color
FORGCL	EQU	'F'		;set foreground color
BACKGC	EQU	'B'		;set background color
FBCOL	EQU	'C'		;set foreground and background color
FNTSEL	EQU	'G'		;select font
;
;-----------------------------------------------------------------------------
;
;       Video Modes commands
;
VCHMD	EQU	0		;video Character Mode comm.
VGRMD	EQU	1		;video Graphic Mode command
VUSMD	EQU	2		;video User Mode command
;
;****************************************************************
;*                                                              *
;*                  PORTS EQU DEFINITIONS                       *
;*                                                              *
;****************************************************************
;
S1CW5	EQU	SIOTRE+SIOTR8+SIORTS	;command word for SIO 1 B
					;register 5
;
CTC13M	EQU	CTCRES+CTCIEN+CTCCNT+CTCLTC
					;mode word for CTC 1 channel 3
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
;	Include Floppy disk service procedures
;
;
;                ***********************************
;                *                                 *
;                *        Z256 Microcomputer       *
;                *                                 *
;                ***********************************
;                *                                 *
;                *   FLOPPY Procedures Ver  3.0    *
;                *                                 *
;                *   Copyright (C) 1988 by Petr    *
;                *                                 *
;                *     Last UpDate: 12.09.1988     *
;                *                                 *
;                ***********************************
;
;
;****************************************************************
;*                                                              *
;*                  EQU DEFINITIONS                             *
;*                                                              *
;****************************************************************
;
FIOTO	EQU	200		;floppy disk time out constant in 10ms
;
;****************************************************************
;*                                                              *
;*                  FLOPPY DISK MAIN ROUTINES                   *
;*                                                              *
;****************************************************************
;
;	SDRIVE procedure - select driver
;
;	Input:  HL - begin of driver parameters table
;	Output: none
;
;	Format of driver parameter table
;
;	HL      - DRVCHR ... driver characteristic
;	HL - 1  - DRVMSK ... driver mask
;	HL - 2  - LSTSEC ... last sector on track
;	HL - 3  - MONDL  ... motor on delay constant
;	HL - 4  - MOFFDL ... motor off delay constant
;
SDRIVE:	LD	DE,DRVCHR	;get begin of DPT in 2114
	LD	BC,0005H	;get length of DPT
	LD	A,(DE)		;get old DRVCHR
	XOR	(HL)		;compare with new DRVCHR
	EX	AF,AF'		;save it
	LDDR			;copy table for new disk
;
	LD	A,(DRVMSK)	;get new DRVMSK
	LD	B,A		;save it to B
	AND	03H		;mask driver number
	LD	D,A		;save it to D
	IN	A,(PIO2BD)	;get old DRVMSK
	LD	C,A		;save it to C
	AND	03H		;mask driver number
	CP	D		;if the different driver number
	JR	NZ,SDRV1	;then skip
				;else the same driver
	LD	A,C		;get old DRVMSK
	OR	0EFH		;mask Motor On signal
	AND	B		;reset Motor On in new DRVMSK if needed
	OUT	(PIO2BD),A	;set new DRVMSK
;
	EX	AF,AF'		;get compared	DRVCHRs'
	BIT	6,A		;if the same Double Step flags
	RET	Z		;then end
	JR	RESTOR		;else seek to track 00
;
SDRV1:	LD	H,LTRKTB/256	;get MSB addr of last track table
	ADD	A,LTRKTB & 255;get LSB of old driver track address
	LD	L,A		;HL - complete old driver track addr.
	IN	A,(FDCTRK)	;get old driver track
	LD	(HL),A		;save it to table
;
	LD	A,CTCRES	;get CTC reset command
	OUT	(CTC13),A	;stop Motor Off time out
;
	LD	A,B		;get new DRVMSK
	RES	4,A		;set Motor Off
	OUT	(PIO2BD),A	;set new DRVMSK
	LD	A,D		;get new driver number
	ADD	A,LTRKTB & 255;get LSB of new driver track addr.
	LD	L,A		;HL - complete new driver track addr.
	LD	A,(HL)		;get track of new drive
	OUT	(FDCTRK),A	;set it to FDC
	INC	A		;test if valid
	RET	NZ		;then return
				;else perform RESTOR
;
;-----------------------------------------------------------------------------
;
;	RESTOR procedure - set driver head to track 00
;
;	Input:	none
;	Output: none
;
RESTOR:	LD	A,10		;get Motor Off time out cycles
	LD	(CYCLES),A	;reset Motor Off time out
;
	LD	D,FRESTR+FHEADL	;get FDC restore command
	CALL	SETSTR		;set step rate for FDC command
	LD	A,D		;get FDC command to A
	CALL	PERFUN		;perform FDC function
	JR	SEEKT6		;restore DRVMSK and return
;
;-----------------------------------------------------------------------------
;
;	SEEKTR procedure - seek head to desired track
;
;	Driver and old track in advance selected
;
;	Input: L - desired track number
;
SEEKTR:	LD	A,10		;get Motor Off time out cycles
	LD	(CYCLES),A	;reset Motor Off time out
;
	LD	A,(DRVCHR)	;get driver characteristic
	LD	C,A		;save it to C
	RLCA			;test Sides flag
	LD	A,FSIDE1	;prepare Side 1 select mask
	JR	NC,SEEKT1	;if Sides = 0 then one side - skip
	SRL	L		;else bit0 of Track to CY, Track/2
	JR	C,SEEKT2	;if Side 1 then skip
SEEKT1:	XOR	A		;else Side 0 select
SEEKT2:	LD	(FLSIDE),A	;save Side select for Read/Write
;
	IN	A,(FDCTRK)	;get actual head position
	SUB	L		;subtract desired track - get	offset
	RET	Z		;if zero - all done - return
	LD	D,FSTOUT+FHEADL	;prepare Step Out command
	JR	NC,SEEKT3	;if des. track < actual track then skip
	LD	D,FSTIN+FHEADL	;else Step In command
	NEG			;maks offset positive
;
SEEKT3:	BIT	6,C		;test Double Step flag
	JR	Z,SEEKT4	;if no double then skip
	ADD	A,A		;else multiply by two
SEEKT4:	LD	B,A		;save step counter to B
;
	CALL	SETSTR		;set step rate for command
;
SEEKT5:	LD	A,D		;get step FDC command
	CALL	PERFUN		;perform it
	DJNZ	SEEKT5		;repeat for all steps
;
	LD	A,L		;get dest. track number
	OUT	(FDCTRK),A	;set it to FDC
;
SEEKT6:	LD	A,H		;get original driver mask
	OUT	(PIO2BD),A	;restore orig. mask
	RET			;
;
;-----------------------------------------------------------------------------
;
;	RDADR procedure - Read head address
;
;	Driver and track in advance selected.
;
;	Input:	A' - page mask - only for write cycle (M1 and Read
;                                cycle must be set to page 3)
;		HL - buffer start address
;
;	Output: A  = 00H successful operation
;		A  = xxH FDC error in operation
;		A  = FFH Time Out error
;		HL - buffer end address + 1
;		     (only if successfull operation)
;
RDADR:	LD	B,FRDADR	;B - read track FDC command
;
	DEFB	011H		;LD DE,xxxx destroy next LD B,nn instruction
;
;-----------------------------------------------------------------------------
;
;	RDTRK procedure - Read entire track from floppy disk
;
;	Driver and track in advance selected.
;
;	Input:	A' - page mask - only for Write cycle (M1 and Read
;                                cycle must be set to page 3)
;		HL - buffer start address
;
;	Output: A  = 00H successfull operation
;		A  = xxH FDC error in operation
;		A  = FFH Time Out error
;		HL - buffer end address + 1
;		     (only if successfull operation)
;
RDTRK:	LD	B,FRDTRK	;B - read track FDC command
	LD	C,NBINI		;C - next byte of INI instruction
;
RDTRK1:	LD	DE,1		;one cycle for that command
	JR	FLOPIO		;perform I/O on disk
;
;-----------------------------------------------------------------------------
;
;	FRMTRK procedure - format track
;
;	Driver and track in advance selected.
;
;	Input:  HL - buffer start address
;
;	Output: A  = 00H successful operation
;		A  = xxH FDC error in operation
;		A  = FFH Time Out error
;		HL - buffer end address + 1
;		     (only if successful operation)
;
FRMTRK:	LD	BC,256*FWRTRK+NBOUTI
				;B - format track FDC command
				;C - next byte of OUTI instruction
	LD	A,M1MP3+WRMP3+RDMP3
				;select page 3 for all cycles
	EX	AF,AF'		;save selector to A'
	JR	RDTRK1		;perform I/O on disk
;
;-----------------------------------------------------------------------------
;
;	FWRITE procedure - Write track to floppy disk
;
;	Driver and track in advance selected.
;
;	Input:	D  - first sector number
;		HL - buffer start address
;
;	Output: A  = 00H successful operation
;		A  = xxH FDC error in operation
;		A  = FFH Time Out error
;		HL - buffer end address + 1
;		     (only if successful operation)
;
FWRITE:	LD	BC,256*(FWRSEC+FMSEC)+NBOUTI
				;B - write multisector FDC command
				;C - next byte of OUTI instruction
	LD	A,M1MP3+WRMP3+RDMP3
				;select page 3 for all cycles
	EX	AF,AF'		;save selector to A'
	DEFB	03EH		;LD A,xx - skip next LD BC,nnnn op. code
				;NBINI = AND D
				;FRDSEC+FMSEC = SBC A,B
;
;-----------------------------------------------------------------------------
;
;	FREAD procedure - read data from one track of floppy disk
;
;	Driver and track in advance selected.
;
;	Input:  A' - page mask - only for Write cycle (M1 and Read
;			         cycle must be set to page 3)
;		D  - first sector number
;		HL - buffer start address
;
;	Output: A  = 00H successful operation
;		A  = xxH FDC error in operation
;		A  = FFH Time Out error
;		HL - buffer end address + 1
;		     (only if successful operation)
;
FREAD:	LD	BC,256*(FRDSEC+FMSEC)+NBINI
				;B - read multisector FDC command
				;C - next byte of INI instruction
;
DISKIO:	LD	A,(LSTSEC)	;get last sector on track
	SUB	D		;subtract first sector number
	INC	A		;A = number of I/O sectors
	LD	E,A		;save it to E
;
;-----------------------------------------------------------------------------
;
;	FLOPIO procedure - perform basic Input/Output data on to floppy
;
;	Driver and track in advance selected
;
;	Input:	A' - page mask - only for Write cycle (M1 and Read
;                                cycle must be set to page 3)
;		B  - FDC type II or III command
;		C  - next byte of INI or OUTI instruction
;		D  - first sector number (for type II FDC command)
;		D  = 0 for type III FDC command
;		E  - number of I/O sectors (for type II FDC	command)
;		E  = 1 for type III FDC command
;		HL - buffer start address
;
;		CTC13 must be in in reset or active state
;
;	Output: A  = 00H successful operation
;		A  = xxH FDC error in operation
;		A  = FFH Time Out error
;		HL - buffer end address + 1
;		     (only if successful operation)
;
FLOPIO:	LD	A,CTC13M	;get mask for CTC13
	OUT	(CTC13),A	;reset CTC13 timer
;
	LD	A,D		;get first sector number
	OUT	(FDCSEC),A	;set it to FDC
;
	CALL	MOTUP		;start up motor in driver
;                               
	LD	(STACKP),SP	;Save stack pointer into 2114
	LD	SP,TEMPSP	;set temporary stack to 2114
;
	LD	A,M1MP3+WRMP3+RDMP3
	OUT	(BANK),A	;select page 3 for all cycles
;
	LD	A,C		;get next byte INI/OUTI to A
	LD	(FTRNSF+1),A	;set it to transfer routine
;
	LD	C,FDCDAT	;prepare FDC data reg. address
	LD	IX,DIOLP	;prepare jump address
;
	LD	A,FIOTO		;get floppy I/O Time Out
	OUT	(CTC13),A	;start Time Out interval
;
	XOR	A		;prepare O.K. flag
	EX	AF,AF'		;to A' , get page for operation
	OUT	(BANK),A	;set page
;
	LD	A,(FLSIDE)	;get floppy side
	OR	B		;add FDC command
	OUT	(FDCSCR),A	;Start command
;
	CALL	DIOTRN		;perform I/O operation
;
	IN	A,(FDCSCR)	;get FDC status
	RRCA			;if not BUSY
	JR	NC,FLIO1	;then skip
;
	LD	A,FABORT	;else get abort FDC command
	OUT	(FDCSCR),A	;send it to FDC
;
FLIO1:	LD	A,CTCRES	;stop Time Out
	OUT	(CTC13),A	;
;
	XOR	A		;set page 0
	OUT	(BANK),A 	;	
;
	LD	SP,(STACKP)	;restore old stack pointer
;
	LD	A,(DRVMSK)	;get driver mask
	RLCA		;test bit 7 - Motor Switch
	JR	NC,FLIO2	;if zero - no switch motor off
;
	LD	A,(MOFFDL)	;get Motor Off delay
	OR	A		;test if delay required
	JR	Z,FLIO3		;no - skip
;
	LD	B,A		;save MOFFDL
	LD	A,10		;set 100ms interval
	LD	(CYCLES),A	;set 10 cycles
	LD	A,CTC13M	;get mask for CTC13
	OUT	(CTC13),A	;preset CTC13 timer
	LD 	A,B		;restore MOFFDL
	OUT	(CTC13),A	;start interval
	EI			;enable timer interrupts
;
FLIO2:	CALL	WFDCRY		;wait to FDC ready status
	LD	B,A		;save FDC status
	EX	AF,AF'		;get Time Out status
	OR	B		;sum both status
	RET			;return with error status
;
FLIO3:	LD	A,(DRVMSK)	;get driver mask
	RES	4,A		;set Motor Off
	OUT	(PIO2BD),A	;
	JR	FLIO2
;
;-----------------------------------------------------------------------------
;
;	SETSTR procedure - set step rate for FDC command
;
;	Input:	D  - FDC command
;	Output: D  - FDC command with set step rate
;		H  - old driver mask
;	Uses:	A,E
;
SETSTR:	LD	A,(DRVCHR)	;get driver characteristic
	LD	E,A		;save it to E
	AND	03H		;mask step rate
	OR	D		;get complete command
	LD	D,A		;set it to reg. D
;
	IN	A,(PIO2BD)	;get actual driver mask
	LD	H,A		;save it to H reg.
	LD	A,E		;get driver characteristic
	AND	08H		;mask step clock
	XOR	H		;get new mask for stepping
	OUT	(PIO2BD),A	;set it to PIO
	RET
;
;-----------------------------------------------------------------------------
;
;	RDSYS procedure - read first sector (Loader) from floppy
;
;	Input:  none
;	Output: none
;
RDSYS8:	INC	C		;next drive number
	DJNZ	RDSYS1		;repeat for all drives
;
	LD	HL,BTERMS	;get Boot Error message
RDSYS7:	CALL	STROUT		;print it
;
RDSYS:	LD	BC,0400H	;B = 4 ... number of disks, C = disk 00
;
RDSYS1:	LD	HL,DSKTYP	;get Disk Types table address
;
RDSYS2:	LD	A,(HL)		;get disk type
	OR	A		;if end of table
	JR	Z,RDSYS8	;then new disk - skip
;
	OR	C		;get DSKMSK
	EXX		;save BC and HL
;
;	LD	(DRVMSK),A	;set new Driver mask
	CALL	MOTUP0	;start up motor in driver if needed
;
	LD	A,FABORT	;get FDC reset command
	OUT	(FDCSCR),A	;reset FDC status register
;
	LD	BC,80	;set loop counter for Index pulse detection
			;81*256*9.75 micro sec. > 202ms
;
	IN	A,(FDCSCR)	;get FDC status
	LD	D,A		;save old status
RDSYS3:	IN	A,(FDCSCR)	;get FDC status
	CP	D		;if Index pulse edge occur
	JR	NZ,RDSYS5	;then skip from loop
	LD	D,A		;set old status
	DJNZ	RDSYS3		;else
;
	DEC	C		;if time not left
	JR	NZ,RDSYS3	;then wait
;
RDSYS4:	EXX			;restore BC and HL
	INC	HL		;next type of disk
	JR	RDSYS2
;
RDSYS5:	CALL	RESTOR		;seek to track 00
;
	LD	A,M1MP3+WRMP0+RDMP3
	EX	AF,AF'		;prepare page for read Loader
	LD	D,1		;start sector 1
	LD	HL,LOADER	;set begin of Loader
	PUSH	HL		;save begin of Loader
	CALL	FREAD		;read Loader
	POP	DE		;restore begin of Loader to reg. DE
				;status is already in flags
	JR	NZ,RDSYS4	;if bad - skip to another type
	EX	DE,HL		;DE = Loader stop addr., HL = begin addr.
	LD	A,(HL)		;init check sum
	INC	HL		;next byte address
	CP	0E5H		;if erased data
	JR	Z,RDSYS9	;then skip
;
RDSYS6:	ADD	A,(HL)		;add byte
	AND	A		;clear carry
	INC	HL		;next byte address
	SBC	HL,DE		;test if end address
	ADD	HL,DE		;restore byte address
	JR	NZ,RDSYS6  	;repeat for entire loader
	AND	A		;test check sum
	JP	Z,LOADER	;yes - skip to Loader
;
RDSYS9:	LD	HL,NSYSMS	;get Non System Disk message
	JR	RDSYS7		;next try
;
;-----------------------------------------------------------------------------
;
;	MOTUP procedure - start up motor in driver
;
;	Input:  none
;	Output: none
;	Uses:   A,D
;
MOTUP:	LD	A,(DRVMSK)	;get driver mask
	RLCA			;test motor switch flag
	RET	NC		;if zero - return
;
	IN	A,(PIO2BD)	;get actual driver mask
	BIT	4,A		;test Motor On signal
	RET	NZ		;if active - return
;
MOTUP0:	OR    MOTON		;set Motor On signal to log.1
	OUT	(PIO2BD),A	;set Motor On
;
	LD	A,(MONDL)	;get Motor On delay
	LD	D,A		;prepare it to D reg.
;
MOTUP1:	IN	A,(CTC12)	;get counter
	DEC	A		;test if near to zero
	JR	Z,MOTUP1	;yes - skip & wait to change constant
;
MOTUP2:	IN	A,(CTC12)	;get counter
	DEC	A		;test if near to zero
	JR	NZ,MOTUP2	;no - skip & wait to zero
;
	DEC	D		;if MOTON*10ms interval not left
	JR	NZ,MOTUP1	;then wait - skip
;
	RET			;else return
;
;-----------------------------------------------------------------------------
;
;	PERFUN procedure - perform floppy disk function
;
;	Input:  A  - FDC command
;
;	Output: A  - FDC status
;
PERFUN:	OUT	(FDCSCR),A	;send command to FDC
;
;-----------------------------------------------------------------------------
;
;	WFDCRY procedure - wait to FDC complete operation
;
;	Output: A  - FDC status
;
WFDCRY:	LD	A,6		;dummy loop for
WFDCLP:	DEC	A		;busy bit valid
	JR	NZ,WFDCLP	;work for 5"SD and 4MHz clock
;
WFDCR1:	IN	A,(FDCSCR)	;get status of FDC
	BIT	0,A		;test bit 0 = BUSY
	JR	NZ,WFDCR1 	;if non zero than busy - wait
;
	RET			;else function complete
;
;****************************************************************
;*                                                              *
;*                   FOLLOWED AREA MUST BE                      *
;*               TRANSFERRED IN TO RAM TO PAGE 3                *
;*                                                              *
;****************************************************************
;
BTA3:			;begin of transposed area
;
	PHASE	WORKA3	;begin of work area in page 3
;
;-----------------------------------------------------------------------------
;
;	DIOTRN routine - floppy disk input/output data transfer
;
;   This routine work for read, write and formating 8"SD, 5"DD with system
; clock 2MHz and for all format floppy disks without formating 5"SD with
; system clock 4MHz.
;
;	Input:	A  - EPROFF command
;		C  - FDCDAT port address
;		E  - number of physical I/O sectors
;		HL - DMA address
;		IX - DIOLP address
;		IY - Time Out error service procedure address
;
DIOTRN:	EI			;enable interrupts
	HALT			;wait for first byte transferring
	HALT			;wait for next byte transferring
;				(due to write operation)
DIOLP:	JR	C,DIOTST	;if CY=1 - one sector completed
	SCF			;set Carry
	RRA			;shift CY to bit 7 reg.A. bit 0 to CY
	JP	(IX)		;loop to DIOLP
;
DIOTST:	DEC	E		;if another sector to read/write
	JR	NZ,DIOTRN	;then repeat
	RET			;else all sectors completed
;
;-----------------------------------------------------------------------------
;
;	FTRNSF routine - floppy disk data transfer interrupt
;
;	Input:	C  - FDCDAT port address
;		HL - DMA address
;
;	Output: A  = 0
;		CY = 0
;		B  = B - 1
;
FTRNSF:	INI			;INI or OUTI byte transfer operation
				;HL=HL+1,B=B-1
	XOR	A		;clear Carry and reg. A
	EI			;enable next interrupt
	RETI
;
;-----------------------------------------------------------------------------
;
;	FDCTO routine - floppy disk Time Out interrupt service
;
FDCTO:	POP	BC		;abort return address
	LD	A,0FFH		;get time out error flag
	EX	AF,AF'		;save it
	RETI			;restore interrupt daisy chain and
				;go to service subroutine
;
;-----------------------------------------------------------------------------
;
	DEPHASE
;
LTA3	EQU	$-BTA3	;length of transposed area
;
;****************************************************************
;*                                                              *
;*                        END OF AREA                           *
;*                TRANSFERRED IN TO RAM PAGE 3                  *
;*                                                              *
;****************************************************************
;
;
;****************************************************************
;*                                                              *
;*                     FOLLOWED AREA MUST BE                    *
;*              TRANSFERRED IN TO RAM BEHIND BIOS               *
;*                                                              *
;****************************************************************
;
BTAREA:			;begin of transposed area
;
	PHASE	WEBIOS
;
;-----------------------------------------------------------------------------
;
;	TIMINT routine - timer interrupt service - Motor Switch
;
TIMINT:	PUSH	AF		;save AF
	LD	A,(CYCLES)	;get no of cycles
	DEC	A		;one cycle less
	LD	(CYCLES),A	;save new cycles counter
	JR	NZ,TIMIN1	;if no last cycle - skip
;
	LD	A,CTCRES	;else stop CTC13
	OUT	(CTC13),A	;interrupts
	IN	A,(PIO2BD)	;get driver mask
	XOR	MOTON		;invert Motor On signal
	OUT	(PIO2BD),A 	;set new driver mask
;
TIMIN1:	POP	AF		;restore AF
	EI			;enable next interrupts
	RETI			;restore interrupt daisy chain
;
CYCLES:				;cycles counter
;
;-----------------------------------------------------------------------------
;
	DEPHASE
;
LTAREA	EQU	$-BTAREA	;length of transposed area
;
;****************************************************************
;*                                                              *
;*                       END OF AREA                            *
;*             TRANSFERRED IN TO RAM BEHIND BIOS                *
;*                                                              *
;****************************************************************
;
;****************************************************************
;*                                                              *
;*              DISK TYPE TABLE FOR BOOT                        *
;*                                                              *
;****************************************************************
;
DSKTYP:	DEFB	FIVE +DDENS+MOTON	;5",DD (MOTON is dummy)
	DEFB	EIGHT+SDENS		;8",SD
	DEFB	EIGHT+DDENS		;8",DD
	DEFB	0			;end of table
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
;	Include CRT service procedures
;
;                ***********************************
;                *                                 *
;                *        Z256 Microcomputer       *
;                *                                 *
;                ***********************************
;                *                                 *
;                *   VIDEO  Procedures  Ver 3.0    *
;                *                                 *
;                *      Copyright (C) 1988 by      *
;                *                                 *
;                *       Petr, Honza, Zdenek       *
;                *                                 *
;                *    Last UpDate: 13.09.1988      *
;                *                                 *
;                ***********************************
;
;****************************************************************
;*                                                              *
;*              MAIN CRT PROGRAM - CONSOLE OUTPUT               *
;*                                                              *
;****************************************************************
;
CONOUT:	LD	A,C		;prepare char to A for testing
	LD	HL,(ROUTER)	;load router
	JP	(HL)		;jump to procedure
;
;-----------------------------------------------------------------------------
;
;	WR1CH - this procedure dispay one character
;		even control character
;
;	Input:	C  - char
;
WR1CH:	CALL	SETSEL		;set router back to SELECT
	JR	WRCHAR		;write one character
;
;-----------------------------------------------------------------------------
;
;       SELECT - this procedure select the chars or CTRL codes
;
;       Input:  A  - char
;
SELECT:	CP	SPACE		;if A < SPACE
	JR	C,CTRLSL	;then goto control select
	CP	DEL		;else if A = DEL
	JR	Z,DELETE	;then goto  delete character
				;else write char to screen
;
;-----------------------------------------------------------------------------
;
;	WRCHAR - this procedure writes a char to screen
;
;	Input:	C  - ASCII char
;
WRCHAR:	LD	HL,(MA)		;get MA of char
	LD	A,H		;MSB of PA  calculation
	ADD	A,A		;shift MSB
	ADD	A,A		;to the left
	ADD	A,A		;4 times
	ADD	A,A		;low nibble = 0
	OR	80H		;set most bit of PA
	LD	D,A		;set PA
	LD	E,L		;to the DE reg. pair
	INC	HL		;MA for next char
	LD	(MA),HL		;save new MA
	EX	DE,HL		;set PA to HL
;
	LD	A,(ACTFNT)	;get MSB of char address
	LD	D,A		;set it to D
	EX	AF,AF'		;and save it to A'
	LD	E,C		;DE = address of char in font
	LD	C,H		;save MSB of PA to C
	CALL	MOVECH		;write char to screen
;
	LD	A,(X)		;get actual pos. of cursor
	INC	A		;move it to right
	LD	(X),A		;save new pos.
	CP	XMAX		;if new pos >= XMAX
	JR	NC,WRCHR1	;then goto new line
				;else update cursor
;
;-----------------------------------------------------------------------------
;
;	UPDCRS - this procedure updates cursor position onto screen
;
UPDCRS:	LD	BC,2*256+14	;B=no. of bytes, C=updated reg. of CRTC
	LD	DE,MA+1		;address of high cursor addr.
UPCRR1:	LD	HL,8000H	;even addr for regs. select
	LD	A,WRMP3+M1MP1	;write to page 3 - Motorola reg's
	OUT	(BANK),A	;select page
	OUT	(SBOARD),A	;select video board
UPCRR2:	LD	(HL),C		;select register no.
	LD	A,(DE)		;get register content
	INC	L		;odd address
	LD	(HL),A		;set register
	DEC	L		;prepare even address
	DEC	DE		;next register content
	INC	C		;next register no.
	DJNZ	UPCRR2		;repeat all set reg's
	OUT	(MBOARD),A	;select RAM board
	XOR	A		;page 0
	OUT	(BANK),A	;select page 0
	RET
;
;-----------------------------------------------------------------------------
;
WRCHR1:	XOR	A		;else new pos X = 0
	LD	(X),A		;set it
;
;-----------------------------------------------------------------------------
;
LFEED:	LD	A,(Y)		;get the line no.
	INC	A		;next line
	CP	YMAX		;if line no. < YMAX
	JR	C,LFEED1	;then goto set cursor
				;else scroll
;
;-----------------------------------------------------------------------------
;
;	SCROLL - shift screen one line up and make new VMOFF
;
SCROLL:	LD	HL,(VMOFF)	;get the current VMOFF
	LD	BC,XMAX		;get the number of chars/line
	ADD	HL,BC		;add it to VMOFF
	LD	(VMOFF),HL	;save new VMOFF
	LD	DE,YMAX*XMAX	;get size of screen
	ADD	HL,DE		;HL - start address of new line
	LD	B,C		;prepare num of chars to delete - XMAX
	CALL	CLREL1		;make erase to end of line of new line
;
;-----------------------------------------------------------------------------
;
;	UPDBC - this procedure updates Base and Cursor registers in Motorola
;
UPDBC:	LD	BC,4*256+12	;B=no. of reg. update, C=first CTRC reg. no
	LD	DE,VMOFF+1	;set addr. of content of first reg.
	JR	UPCRR1		;update it
;
;-----------------------------------------------------------------------------
;
LFEED1:	LD	(Y),A		;save new line no.
	JR	UPDCRS		;and update cursor
;
;-----------------------------------------------------------------------------
;
;	DELETE - this procedure delete a character left from cursor
;
DELETE:	CALL	MAKEBS		;shift cursor to the left one char
	LD	B,1		;one character delete
	JP	MKCLE1		;delete it
;
;-----------------------------------------------------------------------------
;
;	CTRLSL - selects the CTRL commands
;
;	Input:	A  - char
;
CTRLSL:	CP	CR		;if CR
	JR	Z,MAKECR	;then goto make CR
;
	CP	LF		;if LF
	JR	Z,MAKELF	;then goto make LF
;
	LD	HL,ESCSLC	;prepare router
	CP	ESC		;if ESC
	JR	Z,SETRTR	;then goto set ESC router
;
	CP	BSPACE		;if BS
	JR	Z,MAKEBS	;then goto make BS
;
	CP	CRLF		;if CR and LF (^_)
	JR	Z,MKCRLF	;then goto make CR and LF
;
	CP	CLRSVM		;if ClrScr
	JR	Z,CLRSCR	;then goto clear screen
;
	RET
;
;-----------------------------------------------------------------------------
;
;	MKCRLF - this procedure makes both CR and LF
;
MKCRLF:	CALL	MAKECR	;first make CR
;
;-----------------------------------------------------------------------------
;
;	MAKELF - this procedure makes the LF.
;
MAKELF:	LD	BC,XMAX		;next MA is XMAX chars further in
	LD	HL,(MA)		;memory, to calculated
	ADD	HL,BC		;add XMAX + MA
	LD	(MA),HL		;and save it
	JR	LFEED		;make lfeed or scroll
;
;-----------------------------------------------------------------------------
;
;	MAKECR - this procedure makes the CR.
;
MAKECR:	LD	A,(X)		;get the X position of cursor
	LD	C,A		;make in BC the integer address of CRS
	XOR	A		;zero ACC
	LD	B,A
	LD	(X),A		;save the X position of cursor
	LD	HL,(MA)		;get the MA address to HL
	SBC	HL,BC		;HL=HL-BC, HL contains the MA address
				;of the begining of current line
MKCR1:	LD	(MA),HL		;save it
	JP	UPDCRS		;update cursor position
;
;-----------------------------------------------------------------------------
;
;	MAKEBS - this procedure makes the BS.
;
MAKEBS:	LD	A,(X)		;get the X pos of CRS
	DEC	A		;if on first position on the line
	RET	M		;then return
	LD	(X),A		;else save new position
	LD	HL,(MA)		;get the MA address
	DEC	HL		;decrease it
	JR	MKCR1		;save it and update new cursor position
;
;-----------------------------------------------------------------------------
;
;	CLRSCR - this procedure clear the screen
;
CLRSCR:	CALL	CLEARS		;clear page 1 and 2
	LD	HL,X		;get address of X variable
	LD	BC,600H		;B=num of bytes, C=0
CLRVAR:	LD	(HL),C		;clear video variable
	INC	HL		;next variable
	DJNZ	CLRVAR		;repeat for X,Y,MA and VMOFF
	JR	UPDBC		;update VMOFF and MA
;
;
;-----------------------------------------------------------------------------
;
SETSEL:	LD	HL,SELECT	;set router back to SELECT procedure
SETRTR:	LD	(ROUTER),HL	;set new router
	RET
;
;-----------------------------------------------------------------------------
;
;	ESCSLC - select the ESCAPE commands (chars after ESC)
;
;	Input:	A  - char
;
ESCSLC:	LD	HL,SELECT	;set router back to
	LD	(ROUTER),HL	;SELECT routine
;
	CP	CLREOL		;if clear to end of	line
	JP	Z,MKCLEL	;then goto make clear to EOL
;
	LD	HL,GOTOXY	;prepare router for GotoXY command
	CP	GOXY		;if GotoXY
	JR	Z,SETRTR	;then set router to GotoXY
;
	CP	SETUND		;if set underline
	JR	Z,MKUNDL	;then goto make underline
;
	CP	RESUND		;if reset underline
	JR	Z,CLUNDL	;then goto clear underline
;
	CP	INVCOL		;if invert color
	JR	Z,MKINVC	;then goto make invert color
;
	CP	NORMCL		;if original color
	JR	Z,MKNRMC	;then goto make original color
;
	LD	HL,SETFC	;prepare router for set FC
	CP	FORGCL		;if set foreground color
	JR	Z,SETRTR	;then set router to SETFC
;
	LD	HL,SETBC	;prepare router for SETBC
	CP	BACKGC		;if set background color
	JR	Z,SETRTR	;then set router to SETBC
;
	LD	HL,SETFBC	;prepare router for set SETFBC
	CP	FBCOL		;if set fore & background color
	JR	Z,SETRTR	;then set router to SETFBC
;
	LD	HL,SETFNT	;prepare router for set font
	CP	FNTSEL		;if font select
	JR	Z,SETRTR	;then set router to set font
;
	LD	HL,WRCHAR	;prepare router for disp. all chars
	CP	PALLCH		;if display all characters
	JR	Z,SETRTR	;then set router to WRCHAR
;
	LD	HL,WR1CH	;prepare router for display one char
	CP	PONECH		;if display one character
	JR	Z,SETRTR	;then set router to WR1CH
;
	RET
;
;
;****************************************************************
;*                                                              *
;*                      MAIN CRT COMMANDS                       *
;*                                                              *
;****************************************************************
;
;	MKUNDL - this procedure sets underline for next characters
;
MKUNDL:	CALL	PRPUN1		;prepare MSB addr. of underline line to H
	OUT	(BANK),A	;select page
	LD	A,0FFH		;set underline pattern
	OUT	(SBOARD),A	;select video board
STUND1:	LD	(DE),A		;underline one character
	INC	E		;if next character
	JR	NZ,STUND1	;then repeat
	JR	COPUN1		;else exit
;
;-----------------------------------------------------------------------------
;
;	CLUNDL - this procedure resets underline for next char's
;
CLUNDL:	CALL	PREPUN	;prepare MSB source and dest. address
;
COPUND:	OUT	(BANK),A	;select	page
	OUT	(SBOARD),A	;select video board
	LDIR			;copy
COPUN1:	OUT	(MBOARD),A	;select RAM board
	XOR	A		;clear A
	OUT	(BANK),A	;select page 0
	RET
;
;-----------------------------------------------------------------------------
;
;	MKINVC - this procedure sets colors to invert colors
;
MKINVC:	DEFB 03EH		;LD A,xx - set bit 1,0 to log. 1
;
;-----------------------------------------------------------------------------
;
;	MKNRMC - this procedure sets colors to normal (original) colors
;
MKNRMC:	XOR	A		;clear bits 1,0
	LD	(INVSTS),A	;save new invert status
;
;------------------------------------------------------------------------------
;
;	SETCOL - this procedure sets colors on content of variables
;		 FCOLOR, BCOLOR and INVSTS.
;
SETCOL:	LD	A,(BCOLOR)	;get current backgr. color
	LD	B,A		;save it to B
	LD	A,(INVSTS)	;get invert status
	XOR	B		;get low bit of procedure table offset
	LD	C,A		;save it
	EX	AF,AF'		;prepare it for GETADR
	LD	A,(FCOLOR)	;get current foregr. color
	XOR	B		;get most bit of procedure table offset
;
	CALL	GETADR		;get procedure address for page 2
	LD	(MOVCH2+1),HL	;set procedure for page 2
	CALL	GETADR		;get procedure address for page 1
	LD	(MOVCH1+1),HL	;set procedure for page 1
;
	LD	A,C		;get low bit of procedure table offset
	EX	AF,AF'		;prepare it for GETADR
	XOR	A		;set most bit to 0 - only blank procedure
	CALL	GETADR		;get procedure address for page 2
	LD	(WRBL2+1),HL	;set procedure for page 2
	CALL	GETADR		;get procedure address for page	1
	LD	(WRBL1+1),HL	;set procedure for page 1
;
	LD	A,C		;get low bit of procedure table offset
	EX	AF,AF'		;prepare it for GETBLA
	XOR	A		;set most bit to 0 - only blank procedure
	CALL	GETBLA		;get procedure address for page 2
	LD	(CLRS2+1),HL		;set procedure for page 2
	CALL	GETBLA		;get procedure address for page 1
	LD	(CLRS1+1),HL	;set procedure for page 1
	JP	SETSEL		;set select router
;
;-----------------------------------------------------------------------------
;
;	SETFC - this procedure sets foreground color in range 0 - 3
;
;	Input:	C  - bit 1,0 ... foreground color (0 - 3)
;
SETFC:	LD	A,C		;get color to A
	LD	(FCOLOR),A	;save it
	JR	SETCOL		;set new colors
;
;-----------------------------------------------------------------------------
;
;	SETFBC - this procedure sets both fore and background color
;
;	Input:	C  - bit 1,0 ... background color (0 - 3)
;		   - bit 3,2 ... foreground color (0 - 3)
;
SETFBC:	LD	A,C		;get colors
	RRCA			;shift 2 times right
	RRCA			;prepare foreground color
	LD	(FCOLOR),A	;save it and continue
;
;-----------------------------------------------------------------------------
;
;	SETBC - this procedure sets background color in range 0 - 3
;
;	Input:	C  - bit 1,0 ... background color (0 - 3)
;
SETBC:	LD	A,C		;get color to A
	LD	(BCOLOR),A	;save it
	JR	SETCOL		;set new color
;
;-----------------------------------------------------------------------------
;
;	SETFNT - this procedure selects font 0..MAXFNT
;
;	Input:	C  - font number from '0' to 'MAXFNT'
;
SETFNT:	CALL	SETSEL		;set select router
	LD	A,C		;get requested font number to A
	SUB	'0'		;subtract offset
	CP	MAXFNT		;if font no. >= MAXFNT
	RET	NC		;then no change and return
	ADD	A,A		;else *2
	ADD	A,A		;*4
	LD	C,A		;save FN*4
	ADD	A,A		;*8
	ADD	A,C		;FN*12 = FN*YPIXEL
	ADD	A,SYSFNT/256	;add offset of fonts A - new font addr.
	EX	AF,AF'		;save it
	CALL	CLUNDL		;restore old font
	EX	AF,AF'		;restore new font addr.
	LD	(ACTFNT),A	;set new font
;
;-----------------------------------------------------------------------------
;
;	SVUNDL - save underline of new selected font
;
SVUNDL:	CALL	PREPUN	;prepare MSB addresses
	EX	DE,HL	;copy from font to save area
	JR	COPUND	;save underlined line of font to area
;
;
;-----------------------------------------------------------------------------
;
;	MKCLEL - this procedure deletes to end of line starting from
;		 cursor position. Cursor is not moved.
;
MKCLEL:	LD	A,(X)		;get the Xpos of CRS
	LD	B,A		;put it to B
	LD	A,XMAX		;get the num. of columns in line
	SUB	B		;subtract XMAX - X
	LD	B,A		;set character counter
MKCLE1:	LD	HL,(MA)		;get the current MA to HL
;
;-----------------------------------------------------------------------------
;
;	CLREL1 - this procedure deletes to end of line. Delete starts at
;		 the address in HL. This address is of MA type.
;		 B characters is deleted.
;
;	Input:	B  - number of chars.to delete, XMAX - X
;		HL - the start address of MA type
;	Uses:	A,B,C,HL.
;
CLREL1:	LD	A,H		;get MSB of MA for PA generating
	ADD	A,A		;*2
	ADD	A,A		;*4
	ADD	A,A		;*8
	ADD	A,A		;*16
CLREL2:	OR	80H		;set most bit to 1
	LD	C,A		;C,L = PA address
CLREL3:	CALL	WRBLNK		;write one blank character
	DEC	B		;if all character cleared
	RET	Z		;then return
	INC	L		;else next char, if no bound
	JP	NZ,CLREL3	;then skip
	LD	A,C		;else add 10H to MSB of PA
	ADD	A,10H		;for set new PA
	JR	CLREL2		;and repeat to end of line
;
;-----------------------------------------------------------------------------
;
;	GOTOXY - this procedure positions the cursor to XY on the screen
;
; The sequence is { ESC = Y X }. Where ESC...1BH...27
;				       =  ...3DH...61
;				       Y  ...line  +SPACE
;				       X  ...column+SPACE
;
; If X and or Y are out of range the coordinate is rounded to XMAX,YMAX.
;
;	Input:	C  - Y coordinate
;
GOTOXY:	LD	A,SPACE+YMAX-1	;get max bound of Y coord.
	CP	C		;if coord. out of bound
	JR	C,GOXY1		;then use bound
	LD	A,C		;else use coord.
GOXY1:	SUB	SPACE		;subtract offset
	LD	(Y),A		;save Y coord, of cursor
	ADD	A,A		;Y*2
	ADD	A,A		;Y*4
	ADD	A,A		;Y*8
	LD	L,A		;next multiply in HL
	LD	H,0		;
	ADD	HL,HL		;Y*16
	LD	D,H		;save Y*16
	LD	E,L		;to DE
	ADD	HL,HL		;Y*32
	ADD	HL,HL		;Y*64
	ADD	HL,DE		;Y*80 = Y*XMAX
	LD	DE,(VMOFF)	;load VMOFF - page offset
	ADD	HL,DE		;Y*80+VMOFF
	LD	(MA),HL		;save result to MA
	LD	HL,GOXY2	;prepare router for X coord.
	JP	SETRTR		;set router
;
GOXY2:	LD	A,SPACE+XMAX-1	;get max bound of X coord
	CP	C		;if coord. out of bound
	JR	C,GOXY3		;then use bound
	LD	A,C		;else use coordinat
GOXY3:	SUB	SPACE		;subtract offset
	LD	(X),A		;save X coord.
	LD	E,A		;set X coord
	LD	D,0		;to DE pair
	LD	HL,(MA)		;get partial result
	ADD	HL,DE		;MA=Y*80+X+VMOFF
	LD	(MA),HL		;save new MA address
	CALL	UPDCRS		;update cursor
	JP	SETSEL		;set SELECT router for next char
;
;
;****************************************************************
;*                                                              *
;*			CONOUT PROCEDURES                       *
;*                                                              *
;****************************************************************
;
;	PREPUN - this procedure prepares MSB address for SET and RES underline
;
PREPUN:	LD	H,UNDLNS/256	;get MSB of saved underlines
PRPUN1:	LD	A,(ACTFNT)	;get MSB addr of selected font
	ADD	A,UNDLIN	;shift address to line with underline
	LD	D,A		;set MSB of addr. of selected font
	LD	BC,256		;set no. of chars in font
	LD	E,C		;set LSB address of selected font
	LD	L,C		;set LSB address of saved underlines
	LD	A,WRMP1+RDMP1	;copy in page 1
	RET
;
;-----------------------------------------------------------------------------
;
;	GETBLA - this procedure returns procedure address from table
;                BLPRTB on the content reg. A,A'
;
;	Input:	A' - low bit of offset in table
;		A  - zero
;
;	Output: HL - procedure address for CLEARS
;
GETBLA:	LD	HL,BLPRTB	;get begin of table
	JR	GTADR1		;get address
;
;-----------------------------------------------------------------------------
;
;	GETADR - this procedure returns procedure address from table
;		 PROCTB on the content reg. A,A'
;
;	Input:	A' - low bit of offset in table
;		A  - most bit of offset in table
;
;	Output: HL - procedure address for MOVECH or WRBLNK
;
GETADR:	LD	HL,PROCTB	;get begin of table
GTADR1:	LD	DE,0		;clear DE
	RRCA			;most offset bit to CY
	RL	E		;most bit to E
	EX	AF,AF'		;get low bit of offset
	RRCA			;low offset bit to CY
	RL	E		;low bit to E
	EX	AF,AF'		;restore A,A'
	ADD	HL,DE		;add offset
	ADD	HL,DE		;2 times
	LD	E,(HL)		;get LSB of procedure address
	INC	HL		;next byte
	LD	D,(HL)		;get MSB of procedure address
	EX	DE,HL		;procedure address to HL
	RET
;
;-----------------------------------------------------------------------------
;
;	EXPFNT - this procedure expand font from coded form to actual
;		 selected font
;
;	Input:	A' - page select
;		HL - start address of coded font data
;
EXPFNT:	EX	AF,AF'		;get page select mask
FNTEXP:	OUT	(BANK),A 	;select page of Video mem.
	LD	A,(ACTFNT)	;load MSB of char. gen. addr
	LD	D,A		;set high addr
	LD	BC,YPIXEL	;set counter for clear
	LD	E,B		;prepare char.gen. address in DE
	EX	AF,AF'		;save MSB of char.gen. addr.
	XOR	A		;set blank for clearing
	OUT	(SBOARD),A	;select Video mem.
FNTEX1:	LD	(DE),A		;clear space for
	INC	DE		;expanded character generator
	DJNZ	FNTEX1		;repeat for 256 bytes
	DEC	C		;repeat for YPIXEL*256 bytes
	JR	NZ,FNTEX1
;
	LD	C,06H		;skip font table
	ADD	HL,BC
	EX	AF,AF'		;restore MSB of char.gen.addr.
	LD	C,A		;set it in C
FNTEX2:	LD	A,(HL)		;get byte code
	INC	HL		;next byte code
	INC	A		;test if all data expanded ?
	JR	Z,FNTEX6	;yes - return
	DEC	A		;no - restore code
	JR	NZ,FNTEX3	;if character code - skip
	LD	E,(HL)		;else get ASCII code next char.
	INC	HL		;next byte code
	JR	FNTEX2		;continue
;
FNTEX3:	LD	B,A		;save code
	RRCA			;shift 4 times right
	RRCA			;shift 4 times right
	RRCA			;shift 4 times right
	RRCA			;shift 4 times right
	AND	0FH		;mask low nibble - first line
	ADD	A,C		;get MSB address in char.gen.
	LD	D,A		;set complete address in char.gen
	LD	A,B		;get code
	AND	0FH		;mask num. of lines of data of char
	JR	Z,FNTEX5	;if no data - skip
	LD	B,A		;set counter
FNTEX4:	LD	A,(HL)		;get data of character
	INC	HL		;next data byte
	LD	(DE),A		;save data into generator
	INC	D		;next line of character
	DJNZ	FNTEX4		;repeat for all data lines
;
FNTEX5:	INC	E		;next character
	JR	FNTEX2		;repeat for all chars.
;
FNTEX6:	OUT	(MBOARD),A	;select RAM mem.
	OUT	(BANK),A	;set page 0
	JP	SVUNDL		;save underline bytes of font
;
;-----------------------------------------------------------------------------
;
;	SETVMD - this procedure sets video modes and status
;
; Input:   C  - CRTC register table code:
;          C  = 0 - character mode
;          C  = 1 - graphic mode
;          C  = 2 - user mode
;          B  - bit 0 = 0 no change of mode
;                     = 1 set mode on reg. C content
;             - bit 1 = 0 no change
;                     = 1 copy video procedures and variables to RAH
;             - bit 2 = 0 no clear
;                     = 1 clear screen
;             - bit 3 = 0 no copy
;                     = 1 copy and expand system character generator
;
;
SETVMD:	LD	A,B		;get control bits to A
	RRCA			;if bit 0 = 0
	JR	NC,SETVM3	;then skip
;
	PUSH	AF		;save control bits
	LD	A,C		;get video mode command
	LD	DE,CRTUST	;prepare User Mode table address
	CP	VUSMD		;User Mode ?
	JR	Z,SETVM1	;yes - set User Mode
				;no
	LD	DE,CRTGRT	;prepare Graphic Mode table address
	CP	VGRMD		;Graphic Mode ?
	JR	Z,SETVM1	;yes - set Graphic Mode
				;no
	LD	DE,CRTCHT	;set Character Mode
;
SETVM1:	LD	HL,8000H	;even address are register numbers
	LD	BC,256*MOTRGS	;set number of prog. Mot. regs to B
			;reg. pointer set to reg. 0
	LD	A,WRMP3+M1MP1	;set 6845 page
	OUT	(BANK),A	;select page
	OUT	(SBOARD),A	;activate CRT board
SETVM2:	LD	(HL),C		;set register pointer in Motorola
	INC	C		;prepare pointer to next register
	INC	L		;odd address for reg. content
	LD	A,(DE)		;get reg. content
	INC	DE		;next byte in table
	LD	(HL),A		;set reg. content to Motorola
	DEC	L		;even addr. for reg. pointer
	DJNZ	SETVM2		;repeat for all regs.
	OUT	(MBOARD),A	;activate RAM board
	XOR	A		;Page 0, EPROM active
	OUT	(BANK),A
	POP	AF		;restore control bits
;
SETVM3:	RRCA			;if bit 1 = 0
	JR	NC,SETVM4	;then skip
;
	LD	BC,CRTLEN	;else copy video
	LD	DE,VWRKAR	;procedure to RAM
	LD	HL,CRTARR	;
	OUT	(SBOARD),A	;select Video	page
	LDIR			;copy external data and procedures
	OUT	(MBOARD),A	;select RAM page
	PUSH	AF		;save control bits
	CALL	UPDBC		;and update cursor and base registers
	CALL	SETCOL		;set standard color
	POP	AF		;restore control bits
;
SETVM4:	RRCA			;if bit 2 = 0
	JR	NC,SETVM5	;then skip
;
	PUSH	AF		;else save control bits
	CALL	CLRSCR		;clear screen
	POP	AF		;restore control bits
;
SETVM5:	RRCA			;if bit 3 = 1
	LD	A,WRMP1+RDMP0	;prepare page select for font
	LD	HL,CHGEPR+4	;prepare start address of compressed font
	CALL	C,FNTEXP	;then make the system char. gen.
	JP	SETSEL		;set select router
;
;-----------------------------------------------------------------------------
;
; 	RVIDEO procedure - Read bytes from Video Board
;
; Procedure for reading bytes from 0.,1.,2. or 3. page on
; Video Board.
;
; Input:  A' - page select
;         BC - number of bytes for transfer
;         DE - destination address in operating RAM
;         HL - source address in Video memory
;
RVIDEO:	EX	AF,AF'		;get page select
	OR	WRMP1		;disable writing to page 0 on Video board
	EX	AF,AF'		;save page select
	CALL	TRNSPR		;prepare address for transfer
RVIDLP:	LD	DE,COPBUF	;set start address of buffer
	CALL	TRNSBT		;transfer bytes from video mem. to 2114
	LD	HL,COPBUF	;set start address of buffer
	LDIR			;copy from 2114 to operating RAM
	LD	B,CBUFLN/256	;set BC to buff length
	EX	AF,AF'		;get 512 blocks counter
	EXX			;select next regs' group
	DEC	A		;next block
	JR	NZ,RVIDLP	;repeat for all blocks
	RET
;
;-----------------------------------------------------------------------------
;
;	WVIDEO procedure - Write bytes to Video Board
; Procedure for writing bytes to 0.,1.,2. or 3. page on
; Video Board.
;
; Input:  A' - page select
;         BC - number of bytes for transfer
;         DE - destination address Video memory
;         HL - source address in operating RAM
;
WVIDEO:	CALL	TRNSPR		;prepare address for transfer
WVIDLP:	LD	DE,COPBUF	;set start address of buffer
	LDIR			;copy from 2114 to operating RAM
	EXX			;set first regs' group
	LD	HL,COPBUF	;set start address of buffer
	CALL	TRNSBT		;transfer bytes from video mem. to 2114
	LD	B,CBUFLN/256	;set BC to buff length
	EX	AF,AF'		;get 512 blocks counter
	DEC	A		;next block
	JR	NZ,WVIDLP	;repeat for all blocks
	RET
;
;
TRNSPR:	XOR	A		;zero to acc.
	SRL	B		;B=No of 512 byte blocks
	RLA			;A=MSB of first block length
	PUSH	HL		;save for HL'
	LD	H,A		;save MSB of first block length
	OR	C		;test if first block length = 0
	LD	A,B		;set blocks counter to A
	LD	B,02		;prepare length of first block = 512
	JR	Z,TRNSSK	;if old length of first block is 0 - skip
	INC	A		;else add 1 to blocks counter for testing
	LD	B,H		;restore MSB length of first block
TRNSSK:	PUSH	BC		;save length of first block
	EXX			;set next regs' group
	POP	BC		;set bytes counter
	POP	HL		;set source address in operating RAM
	RET
;
;
TRNSBT:	EX	AF,AF'		;get Bank mask
	OUT	(BANK),A	;set Bank register
	OUT	(SBOARD),A	;select video board
	LDIR			;copy from 2114 to video memory
	OUT	(MBOARD),A	;select RAM
	LD	B,A		;save Bank mask
	XOR	A		;zero mask
	OUT	(BANK),A	;select page 0
	LD	A,B		;restore bank mask
	LD	B,CBUFLN/256	;set BC to buff length
	EXX			;set next regs' group
	RET
;
;-----------------------------------------------------------------------------
;
;	STROUT procedure - String output on console
;
; Input:  HL - begin of zero terminated string
;
STROUT:	LD	A,(HL)		;get character of string
	INC	HL		;next character
	AND	A		;test if terminator
	RET	Z		;yes - return
STROU0:	LD	C,A		;no - character to reg. C
	PUSH	HL		;save HL pointer
	CALL	CONOUT		;output char. on console
	POP	HL		;restore HL
	JR	STROUT		;repeat for all chars.
;
;-----------------------------------------------------------------------------
;
;	PLOT - this procedure sets light for one pixel in graphic mode
;
;	Input:	BC - X coordinate (0..639)
;		DE - Y coordinate (0..287)
;
PLOT:	CALL	MKGRPA		;make PA from coordinates
	JP	DRAWDT		;draw dot and return
;
;-----------------------------------------------------------------------------
;
;	DRAW - this procedure draw line between two points
;
;	Input:	BC - Xa coordinate of first point (0..639)
;		DE - Ya coordinate of first point (0..287)
;		BC'- Xb coordinate of second point
;		DE'- Yb coordinate of second point
;
DRAW:	PUSH	BC		;save Xa
	EXX			;set to b coord.
	POP	HL		;HL=Xa
	XOR	A		;clear CY
	SBC	HL,BC		;HL=Xa-Xb
	JR	NZ,DRAW1	;if (Xa-Xb)<>0 then skip
	CPL			;else A=0FFH
DRAW1:	LD	H,A		;prepare DIR, if DIR=0 then step proc.A
	LD	L,A		;else step proc. B
	PUSH	HL		;save DIR
	JR	NC,DRAW2	;if (Xa-Xb)>=0 then Xa=X2,Xb=X1 (X2>=X1)
	EXX			;else Xa=X1,Xb=X2               (X2>=X1)
DRAW2:	PUSH	DE		;save Y1
	EXX	;set to 	X2,Y2
	POP	HL		;HL=Y1
	XOR	A		;clear CY
	SBC	HL,DE		;HL=Y1-Y2=-Dy
	LD	IY,INCY		;proc.B - increment	Y
	JR	C,DRAW3		;if Dy<0 then increment Y
	SUB	L		;else Dy=-Dy  0-L
	LD	L,A		;save LSB of neg.
	LD	A,0		;prepare zero
	SBC	A,H		;0-H-CY
	LD	H,A		;save MSB of neg.
	LD	IY,DECY		;proc.B - decrement Y
DRAW3:	PUSH	HL		;save -|Dy|
	PUSH	BC		;save X2
	EXX			;set to X1,Y1
	POP	HL		;HL=X2
	XOR	A		;clear CY
	SBC	HL,BC		;HL=X2-X1=Dx
	PUSH	HL		;save Dx
	EXX			;set to X2,Y2
	CALL	MKGRPA		;compute PA2 address and D2 - dest.point
	LD	A,B		;save D2	to
	EX	AF,AF'		;A' reg.
	POP	BC		;set DX to BC, Dx>=0
	POP	DE		;set -Dy to DE,	 -Dy<0
	EX	(SP),HL		;save PA2 to stack, set DIR to HL
	EXX			;set to X1,Y1
	CALL	MKGRPA		;compute PA1 address and D1 - start point
	POP	DE		;set PA2 to DE
	EX	AF,AF'		;get D2
	LD	C,A		;save D2 to C reg.
;
DRAW4:	LD A,C			;if dot address not equal to
	CP	B		;destination dot - D2
	JP	NZ,DRAW5	;then repeat for next dot
	SBC	HL,DE		;else if PAi equal to dest.byte addr. PA2
	ADD	HL,DE		;restore next byte addr. in HL - PAi
	RET	Z		;then repeat for next dot
				;else all done
DRAW5:	CALL	DRAWDT		;draw first point
	EXX			;set to DIR and Dx,Dy
	BIT	7,H		;if DIR <0
	JP	NZ,DRAW6	;then perform procedure B
	ADD	HL,DE		;else DIR>=O => DIR-|Dy|
	EXX			;set to PA address
;
;	increments X coordinate
;
	RRC	B		;shift dot in the right and SB to CY
	JP	NC,DRAW4	;if 1 not shift to MSB then draw dot
	INC	L		;else increment PA
	JP	NZ,DRAW4	;if LSB of PA not zero then draw dot
	LD	A,H		;else add
	ADD	A,10H		;10H to MSB of PA
	LD	H,A		;set new PA
	JP	DRAW4		;increment done - draw dot
;
DRAW6:	ADD	HL,BC		;DIR<0 => DIR+|Dx|
	EXX			;set to PA address
	JP	(IY)		;go to procedure B
;
;****************************************************************
;*                                                              *
;*                       GRAPHIC SUBPROCEDURES                  *
;*                                                              *
;****************************************************************
;
;	INCY - this procedure increments Y coordinate
;
INCY:	INC	H		;next line (low nibble)
	LD	A,H		;if line not overflow
	AND	0FH		;
	JP	NZ,DRAW4	;then draw dot
	LD	A,L		;else increment PA to next character line
	ADD	A,XMAX		;
	LD	L,A		;and save new PA
	JP	C,DRAW4		;if overfl. then O.K.(already incremented)
	LD	A,H		;else
	SUB	10H		;decrement overflow from
	LD	H,A		;MSB of PA and save new PA
	JP	DRAW4		;increment done - draw dot
;
;-----------------------------------------------------------------------------
;
;	DECY - this procedure decrements Y coordinate
;
DECY:	LD	A,H		;get actual line
	DEC	H		;prev line (low nibble)
	AND	0FH		;if actual line	not zero
	JP	NZ,DRAW4	;then draw dot
	LD	A,L		;else decrement PA to prev character line
	SUB	XMAX		;
	LD	L,A		;and save new PA
	JP	C,DRAW4		;if overfl, then O.K.(already decremented)
	LD	A,H		;else
	ADD	A,10H		;increment overflow to
	LD	H,A		;MSB of PA and save new PA
	JP	DRAW4		;decrement done - draw dot
;
;-----------------------------------------------------------------------------
;
;	MKGRPA - this procedure makes the PA address from dot coordinates
;
;	Input:	BC - X coordinate of dot
;		DE - Y coordinate of dot
;
;	Output: HL - PA - procesor address of byte with dot
;		B  - dot pattern for addressed dot
;
MKGRPA:	LD	H,B		;set column number*8 to
	LD	L,C		;HL
	ADD	HL,HL		;column no. *16
	ADD	HL,HL		;column no. *32
	ADD	HL,HL		;column no. *64
	ADD	HL,HL		;column no.  *128
	ADD	HL,HL		;column no.  *256
	LD	A,C		;get LSB of X coord. - dot addr.
	LD	C,H		;save column no. (byte)
	AND	07H		;mask doc address
	ADD	A,DOTTBL & 255	;compute LSB of dot pattern table address
	LD	L,A		;set dot pattern table address to
	LD	H,DOTTBL/256	;HL
	LD	B,(HL)		;get dot pattern
;
;-----------------------------------------------------------------------------
;
;	MKCHPA - this procedure makes the PA address from byte coordinates
;
;	Input:	C  - X coordinate of byte
;		DE - Y coordinate of byte
;
;	Output: HL - PA address of byte
;
;	Register B saved.
;
MKCHPA:	LD	A,E		;get LSB of Y coord
	AND	0F0H		;clear line address
	LD	L,A		;set row address * 16 to
	LD	H,D		;HL
	LD	A,E		;get line address to A
	AND	00FH		;mask line address
	LD	E,L		;save row address * 16 to DE
	ADD	HL,HL		;row address * 32
	ADD	HL,HL		;row address * 64
	ADD	HL,DE		;row address * 80
	LD	D,A		;save line address
	LD	E,B		;save reg B
	LD	B,08H		;set most bit of MA to 1
	ADD	HL,BC		;HL=MA
	LD	A,H		;get MSB of MA
	ADD	A,A		;*2
	ADD	A,A		;*4
	ADD	A,A		;*8
	ADD	A,A		;*16
	OR	D		;add line address
	LD	H,A		;set PA to HL
	LD	B,E		;restore B reg
	RET
;
;-----------------------------------------------------------------------------
;
DOTTBL:	DEFB	10000000B	;dot 0
	DEFB	01000000B	;dot 1
	DEFB	00100000B	;dot 2
	DEFB	00010000B	;dot 3
	DEFB	00001000B	;dot 4
	DEFB	00000100B	;dot 5
	DEFB	00000010B	;dot 6
	DEFB	00000001B	;dot 7
;
;-----------------------------------------------------------------------------
;
;	SETPEN - this procedure sets pen type and line color
;
;	Input:	B  - Pen Type - 0 = line color Overwrite background
;                               1 = line color AND       background
;                               2 = line color OR        background
;                               3 = line color XOR       background
;
;		C  - Line Color - bit 0,1 ... color 0-3
;                  - Invert St. - bit 4 = 1 . invert page 2
;                               - bit 5 = 1 . invert page 1
;
SETPEN:	PUSH	BC		;save color and pen
	LD	BC,DRTBL1	;set length of table 1
	LD	DE,DRAWDT	;set address of Draw Dot procedure in RAM
	LD	HL,DRTAB1	;source addr. of first block
	LDIR			;copy first block
	POP	BC		;restore pen and color
;
	PUSH	BC		;resave pen and color
	LD	IY,256*(WRMP2+RDMP2)+3EH	;prepare LD A,WRMP2+RDMP2
						;if no access to page 1
	AND	A		;clear CY for SETPP
	BIT	1,C		;test bit 2 of color for page 1 procedure
	CALL	SETPP		;set pen procedure for page 1
	PUSH	DE		;save begin of copped table 2
	POP	IX		;to IX reg.
	JR	NC,SETPN1	;if no acces to page 1 then skip
	LD	BC,DRTBL2	;else set length of table 2
	LDIR			;copy table 2
SETPN1:	POP	BC		;restore pen and color
;
	PUSH	BC		;resave pen and color
	BIT	0,C		;test bit 1 of color for page 2
	CALL	SETPP		;set pen procedure for page 2
;
	LD	BC,DRTBL3	;set length of table 3
	LD	HL,DRTAB3	;set source address of	table 3
	LDIR			;copy end of Draw Dot procedure in RAM
	POP	BC		;restore pen and color
;
	LD	A,WRMP1+RDMP1 ;prepare page mask
	BIT	5,C		;if bit 5 <> 0
	CALL	NZ,INVPG	;then invert page 1
	LD	A,WRMP2+RDMP2	;prepare page mask
	BIT	4,C		;if bit 4 <> 0
	CALL	NZ,INVPG	;then invert page 2
	RET
;
;-----------------------------------------------------------------------------
;
;	SETPP - this procedure set pen procedure for selected page
;
SETPP:	JR	NZ,SETPP3	;if color = 1 then skip
	BIT	1,B		;else test pen type bit 1
	JR	NZ,SETPP4	;if pen1=1 then OR or XOR pen - skip
	LD	A,2FH		;else Over or AND pen with col=0 - reset bit
	LD	(DE),A		;set instruction CPL
	INC	DE		;next instruction	
	LD	A,0A6H		;instruction AND (HL)
SETPP1:	LD	(DE),A		;set instruction
	INC	DE		;next instruction
SETPP2:	SCF			;set flag access.to page
	RET
;
SETPP3:	BIT	0,B		;test pen type bit 0
	LD	A,0B6H		;prepare OR pen
	JR	Z,SETPP1	;if pen OVER or OR and Col=1 then set bit
	BIT	1,B		;else test pen type bit 1
	LD	A,0AEH		;prepare XOR pen
	JR	NZ,SETPP1	;if pen XOR and Col=1 then invert bit
				;else pen AND and col=1 then no change
SETPP4:	JR	C,SETPP5	;if second pass, access to page 1
				;and not access to page 2 then skip
	LD	(DRAWDT),IY	;else set page 2 only in first pass or
				;RET in second pass
	LD	IY,0C9H		;prepare RET for next call SETPP
	RET
;
SETPP5:	PUSH	IX		;restore DE to begin of copped
	POP	DE		;table 2
	RET
;
;-----------------------------------------------------------------------------
;
;	DRTAB - this tables contains parts of draw procedure assembled
;		in RAM
;
DRTAB1:	LD	A,WRMP1+RDMP1	;set page 1 mask
	OUT	(BANK),A	;select page 1 for read/write
	LD	A,B		;prepare bit mask to A
	OUT	(SBOARD),A	;select video board
;
DRTBL1	EQU	$-DRTAB1	;DRTAB1 length
;
DRTAB2:	LD	(HL),A		;save drawed dot
	LD	A,WRMP2+RDMP2 ;set page 2 mask
	OUT	(BANK),A	;select page 2 for read/write
	LD	A,B		;prepare bit mask to A
;
DRTBL2	EQU	$-DRTAB2	;DRTAB2 length
;
DRTAB3:	LD	(HL),A		;save drawed dot
	OUT	(MBOARD),A	;select RAM board
	XOR	A		;get page 0 mask
	OUT	(BANK),A	;select page 0
	RET
;
DRTBL3	EQU	$-DRTAB3	;DRTAB3 length
;
;-----------------------------------------------------------------------------
;
;	INVPG - this procedure inverts the graphic page
;
;	Input:	A  - page selector
;
;	Saved:	C
;
SCRBTS	EQU	96		;number of 256 byte blocks of graphic
				;page inverted up to address 0DFFFH
;
INVPG:	OUT	(BANK),A	;select page
	LD	B,SCRBTS	;set block counter
	LD	HL,8000h	;set start of page
	OUT	(SBOARD),A	;select video board
INVPG1:	LD	A,(HL)		;get byte
	CPL			;invert it
	LD	(HL),A		;save inverted byte
	INC	L		;next byte
	JP	NZ,INVPG1	;if no end of block the repeat
	INC	H		;else next block
	DJNZ	INVPG1		;if no last block then repeat
	OUT	(MBOARD),A	;else select RAM board
	XOR	A		;set page 0
	OUT	(BANK),A
	RET
;
;-----------------------------------------------------------------------------
;
;	MWINDW - this procedure moves window to/from data block
;
;	Input:	A' - page select and direction
;		     bit 7 = 0 ... copy from window to data block
;		     bit 7 = 1 ... copy from data block to window
;		B  - window width in bytes
;		C  - X position of upper left hand corner of window
;		DE - Y position of upper left hand corner of window
;		BC'- start address of data block
;		DE'- window hight in lines
;
;       Output: DE - first free byte in data area
;
WTOBL	EQU	127EH		; LD  A,(HL) instructions for copy
				; LD  (DE),A from window to data block
BLTOW	EQU	771AH		; LD  A,(DE) instructions for copy
				; LD (HL),A from data block to window
;
MWINDW:	CALL	MKCHPA		;convert X,Y coordinates to PA in HL
	LD	C,B		;save window width to C reg.
	EXX			;swap to window hight and start of block
	PUSH	BC		;save start of block
	EX	AF,AF'		;get direction and page mask
	AND	A		;test direction DIR=0 set plus flag
	LD	HL,WTOBL	;prepare copy to data block
	JP	P,MWIND1	;if DIR from window to block then skip
	LD	HL,BLTOW	;else get instruction for copy to window
MWIND1:	LD	(COPWN2),HL	;set instructions to RAM
	AND	7FH		;clear bit 7 - EPROM ON in page select
	EXX			;swap to PA and window width
	POP	DE		;set address of data block
	JP	COPWIN		;goto RAM to copying
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
;	Include Port's service procedures
;
;                ***********************************
;                *                                 *
;                *        Z256 Microcomputer       *
;                *                                 *
;                ***********************************
;                *                                 *
;                *  Ports I/O Procedures  Ver 3.0  *
;                *                                 *
;                *   Copyright (C) 1988 by Petr    *
;                *                                 *
;                *     Last UpDate: 15.07.1988     *
;                *                                 *
;                ***********************************
;
;****************************************************************
;*                                                              *
;*              PORTS INITIALIZATION PROCEDURE                  *
;*                                                              *
;****************************************************************
;	PINIT procedure - Port('s) Initialization
; Procedure for programming any I/O devices according to
; initialization table.
;
; Input:  HL - begin of initialization table
;              Format this table:
;                1.byte - no. of bytes writed to port
;                       - if zero - end of table
;                2.byte - port address
;                3.byte - first byte writed to port
;
PINIT:	LD	A,(HL)		;get no. of bytes
	AND	A		;test if zero
	RET	Z		;yes - return
	LD	B,A		;no - set it to reg. B
	INC	HL		;next byte
	LD	C,(HL)		;get port address to reg. C
	INC	HL		;(HL) - first byte for port
	OTIR			;port initialization
	JR	PINIT		;go to next transfer
;
;****************************************************************
;*                                                              *
;*                SERIALL PORT SERVICE PROCEDURES               *
;*                                                              *
;****************************************************************
;
;	SS1B procedure - Read status of channel 1B
;
; Output: A  - status of channel 1B
;
SS1B:	IN	A,(SIO1BC)	;get SIO status reg.0
	RRCA			;check rx flag
	LD	A,0FFH		;prepare ready flag A=FF
	RET	C		;if char ready then return
				;else enable comunication
	LD	HL,SS1TAB	;prepare table address
	CALL	PINIT		;DTR active pulse generate
	RET			;return with not ready flag A=00
;
SS1TAB:	DEFB	SS1TLN		;length of table
	DEFB	SIO1BC		;SIO1B command reg. address
	DEFB	SIOR5		;reg.5
	DEFB	S1CW5+SIODTR	;set SIO 1 DTR to active state
	DEFB	SIOR5		;reg.5
	DEFB	S1CW5		;set SIO 1 DTR to inactive state
;
SS1TLN	EQU	$-SS1TAB-2	;length of table
;
	DEFB	0		;end of table for PINIT
;
;-----------------------------------------------------------------------------
;
;	SI1B procedure - Read character from channel 1B
;
; Output: A - character from channel 1B
;
SI1B:	CALL	SS1B	;get status of channel 1 B
	AND	A	;test ready flag
	JR	Z,SI1B	;if not ready - wait
;
	IN	A,(SIO1BD)	;get char. to A reg.
	RET
;
;-----------------------------------------------------------------------------
;
;	SO1B procedure - Seriall Output to channel 1 B
; Input:  C  - character writed to channel 1B
;
SO1B:	IN	A,(SIO1BC)	;get SIO status reg.0
	AND	04H		;check tx flag
	JR	Z,SO1B		;wait if not ready
	LD	A,C		;set char. in A
	OUT	(SIO1BD),A	;out char.  in SIO
	RET
;
;****************************************************************
;*                                                              *
;*                  KEYBOARD SERVICE PROCEDURES                 *
;*                                                              *
;****************************************************************
;
;	CONST procedure - read status of console
;
; Output: A  - status of console
;         A  = 0 character not ready
;         A  = 0FFH character	ready
CONST:
;	RET
;
;-----------------------------------------------------------------------------
;
;	CONIN procedure - read one character from console
;
; Output: A  - character from console
CONIN:
;	RET
;
;****************************************************************
;*                                                              *
;*                CENTRONIX PORT SERVICE PROCEDURES             *
;*                                                              *
;****************************************************************
;
;	CENTRONIX layout on PIO1 channel A and B
;
;	DATA 0 to 7 - channel B bit 0 to 7 (output active High)
;	BUSY        - channel A bit 0      (input  active High)
;	PE          - channel A bit 1      (input  active High)
;	ERROR       - channel A bit 2      (input  active Low )
;	STROBE      - channel A bit 7      (output active Low )
;
;-----------------------------------------------------------------------------
;
;	PS1 procedure - read status of parallel port 1
;
; Output: A  = 00 if centronix is busy, error or paper end
;         A  = 0FFH if centronix is ready for next character
;
PS1:	IN	A,(PIO1AD)	;get status of Centronix
	RRCA			;shift BUSY bit to CY
	LD	A,0		;prepare busy flag
	RET	C		;BUSY return
	CPL			;prepare ready flag
	RET			;READY return
;
;-----------------------------------------------------------------------------
;
;	PO1 procedure - write character to parallel port 1
;
; Input:  C  - character writed to port 1
;
PO1:	CALL	PS1		;test if Centronix is ready
	AND	A		;test status
	JR	Z,PO1		;wait to ready
	LD	A,C		;get character
	OUT	(PIO1BD),A	;send it to DATA outputs
	XOR	A		;bit 7 = 0
	OUT	(PIO1AD),A	;set STROBE to 0
	CPL			;bit 7 = 1
	OUT	(PIO1AD),A	;restore STROBE to 1
	RET
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
;	Include Start up service procedures
;
;         ***********************************
;         *                                 *
;         *       Z256 Microcomputer        *
;         *                                 *
;         ***********************************
;         *                                 *
;         *    START UP Program  Ver 3.0    *
;         *                                 *
;         *    Copyright (C) 1988 by Petr   *
;         *                                 *
;         *      Last UpDate: 17.07.1988    *
;         *                                 *
;         ***********************************
;
;****************************************************************
;*                                                              *
;*              SYSTEM START UP PROGRAM                         *
;*                                                              *
;****************************************************************
;
;
;	RESET program - start up program for system initialization
;                     - initialize Motorola 6845 CRT controIer
;                       and clear screen
;                     - programed I/O devices: CTC11,CTC12,CTC13
;                                              SIO1B,PIO2B
;                     - set interrupt mode & register CPU
;                     - load CP/M system Loader from diskette and skip in
;
RESET:	LD	SP,BIOSSP	;Stack Pointer initialize
	LD	A,IVTAB/256	;get most byte of intr. vector tab.
	LD	I,A		;set interrupt register CPU
	IM	2		;set interrupt mode 2
;
	LD	HL,CRTARR	;get addr. of external data in Char EPROM
	LD	DE,VWRKAR	;get dest. addr. in work RAM BIOS area
	LD	BC,EDTLEN	;get length of transposed area
	OUT	(SBOARD),A	;select Video
	LDIR			;copy external data
	OUT	(MBOARD),A	;select RAM
;
	LD	BC,0F00H+VCHMD	;CRTC initialize and
	CALL	SETVMD		;set Video character mode
;
	LD	HL,PTABLE	;get address of ports init. table
	CALL	PINIT		;ports init.
;
	LD	HL,SYSMS2	;get address of Version and Date mess.
	CALL	STROUT		;send it to console
	LD	HL,SYSMS1	;get address of System report message
	CALL	STROUT		;send first part to console
	CALL	STROU0		;send second part to console
;
	LD	BC,2		;set overwrite and line color 2
	CALL	SETPEN		;set procedures for Plot and Draw
;
	LD	HL,BTA3		;get begin of area transferred to page 3
	LD	DE,WORKA3	;get begin of work area in page 3
	LD	BC,LTA3		;get length of transferred area
	LD	A,WRMP3		;select page 3
	OUT	(BANK),A	;for writing
	LDIR	;transfer area
;
	LD	HL,FDCTO	;get address of Time Out service proc.
	LD	(CTC1VT+6),HL	;set it for channel 3 of CTC 1
	LD	HL,FTRNSF	;get addr. of floppy disk data transfer proc.
	LD	(PIO2BV),HL	;set it for channel B of PIO 2
;
	XOR	A		;select page 0
	OUT	(BANK),A	;
;
	LD	HL,BTAREA	;get begin of transposed area to page 0
	LD	DE,WEBIOS	;get destination addr. in work EPROM
				;BIOS area
	LD	BC,LTAREA	;get length of transposed area
	LDIR			;transpose
;
	LD	HL,TIMINT	;get addr. of Timer service subroutine
	LD	(CTC1VT+6),HL	;set it for channel 3 of CTC 1
;
;	Loading CP/M Loader from drive 0 - 3
;
;;; change here to make modification for bigger Eprom
;;;
;;;	jp	bootep		;od 0800h kód zavedení systému z Eprom
;;;
	JP	RDSYS		;
;
;****************************************************************
;*                                                              *
;*                  SYSTEM REPORT MESSAGE                       *
;*                                                              *
;****************************************************************
;
SYSMS2:	DEFB	ESC,GOXY,'#3Ver: ',MAINVR,'.',SUBVER
	DEFB	'  Date: ',DAY,'.',MONTH,'.',YEAR,0
;
	DEFB    255,255,255,0DEh,43h	;doplnìní do 2048 bajtù	
;
;=============================================================================
;
;-----------------------------------------------------------------------------
;
STOPE1	EQU	$		;pro test pøesahu 2048 bajtù
;
;-----------------------------------------------------------------------------
;
;                           System Font
;
	ORG	CHGEPR

SYSFNT:	db	8
	db	1
	db	"30"

;informace o znakové sadì (6 bajtù)
;----------------------------------
	db	00,0E6h,05,0B9h,00,08Ch

;znaková data
;------------

;0 - Z v obrysu
	db	09h		;od mikroøádku 0, 9 linek
	db	11111111b
	db	10000001b
	db	11111001b
	db	00010010b
	db	00100100b
	db	01001000b
	db	10011111b
	db	10000001b
	db	11111111b
;
	db	0		;znaèka posuvu
	db	33		;další znak má kód 33
;33 - !
	db	27h
	db	00010000b
	db	00111000b
	db	00111000b
	db	00010000b
	db	00010000b
	db	00000000b
	db	00010000b
;34 - "
	db	13h
	db	01101100b
	db	01001000b
	db	00100100b
;35 - #
	db	27h
	db	00100100b
	db	00100100b
	db	01111110b
	db	00100100b
	db	01111110b
	db	00100100b
	db	00100100b
;36 - $
	db	27h     
	db	00101000b
	db	01111110b
	db	10101000b
	db	01111100b
	db	00101010b
	db	11111100b
	db	00101000b
;37 - %
	db	36h
	db	01100010b
	db	01100100b
	db	00001000b
	db	00010000b
	db	00100110b
	db	01000110b
;38 - &
	db	27h
	db	00011000b
	db	00100100b
	db	00011000b
	db	00110010b
	db	01001100b
	db	01001100b
	db	00110010b
;39 - '
	db	13h
	db	00011000b
	db	00001000b
	db	00010000b
;40 - (
	db	19h
	db	00000100b
	db	00001000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00001000b
	db	00000100b
;41 - )
	db	19h
	db	00100000b
	db	00010000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00010000b
	db	00100000b
;42 - *
	db	35h
	db	00100100b
	db	00011000b
	db	01111110b
	db	00011000b
	db	00100100b
;43 - +
	db	35h
	db	00010000b
	db	00010000b
	db	01111100b
	db	00010000b
	db	00010000b
;44 - ,
	db	73h
	db	00011000b
	db	00001000b
	db	00110000b
;45 - -
	db	51h
	db	01111100b
;46 - .
	db	72h
	db	00011000b
	db	00011000b
;47 - /
	db	36h
	db	00000010b
	db	00000100b
	db	00001000b
	db	00010000b
	db	00100000b
	db	01000000b
;48 - 0
	db	27h
	db	00111000b
	db	01000100b
	db	01001100b
	db	01010100b
	db	01100100b
	db	01000100b
	db	00111000b
;49 - 1
	db	27h
	db	00010000b
	db	00110000b
	db	01010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	01111100b
;50 - 2
	db	27h
	db	00111100b
	db	01000010b
	db	00000010b
	db	00011100b
	db	00100000b
	db	01000010b
	db	01111110b
;51 - 3
	db	27h
	db	00111100b
	db	01000010b
	db	00000010b
	db	00011100b
	db	00000010b
	db	01000010b
	db	00111100b
;52 - 4
	db	27h
	db	00001100b
	db	00010100b
	db	00100100b
	db	01000100b
	db	01111110b
	db	00000100b
	db      00001110b
;53 - 5
	db	27h
	db	01111110b
	db	01000000b
	db	01111100b
	db	00000010b
	db	00000010b
	db	01000010b
	db	00111100b
;54 - 6
	db	27h
	db	00011100b
	db	00100000b
	db	01000000b
	db	01111100b
	db	01000010b
	db	01000010b
	db	00111100b
;55 - 7
	db	27h
	db	01111110b
	db	01000010b
	db	00000100b
	db	00001000b
	db	00010000b
	db	00010000b
	db	00010000b
;56 - 8
	db	27h
	db	00111100b
	db	01000010b
	db	01000010b
	db	00111100b
	db	01000010b
	db	01000010b
	db	00111100b
;57 - 9
	db	27h
	db	00111100b
	db	01000010b
	db	01000010b
	db	00111110b
	db	00000010b
	db	00000100b
	db	00111000b
;58 - :
	db	36h
	db	00011000b
	db	00011000b
	db	00000000b
	db	00000000b
	db	00011000b
	db	00011000b
;59 - ;
	db	37h
	db	00011000b
	db	00011000b
	db	00000000b
	db	00000000b
	db	00011000b
	db	00001000b
	db	00110000b
;60 - <
	db	27h
	db	00000100b
	db	00001000b
	db	00010000b
	db	00100000b
	db	00010000b
	db	00001000b
	db	00000100b
;61 - =
	db	44h
	db	01111110b
	db	00000000b
	db	00000000b
	db	01111110b

;62 - >
	db	27h
	db	00100000b
	db	00010000b
	db	00001000b
	db	00000100b
	db	00001000b
	db	00010000b
	db	00100000b
;63 - ?
	db	27h
	db	00111000b
	db	01000100b
	db	00000100b
	db	00001000b
	db	00010000b
	db	00000000b
	db	00010000b
;64 - @
	db	27h
	db	00111100b
	db	01000010b
	db	10001110b
	db	10010010b
	db	10001110b
	db	01000000b
	db	00111100b
;65 - A
	db	27h
	db	00011000b
	db	00100100b
	db	01000010b
	db	01000010b
	db	01111110b
	db	01000010b
	db	01000010b
;66 - B
	db	27h
	db	01111100b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100010b
	db	00100010b
	db	01111100b
;67 - C
	db	27h
	db	00011100b
	db	00100010b
	db	01000000b
	db	01000000b
	db	01000000b
	db	00100010b
	db	00011100b
 ;68 - D
	db	27h
	db	01111000b
	db	00100100b
	db	00100010b
	db	00100010b
	db	00100010b
	db	00100100b
	db	01111000b
;69 - E
	db	27h
	db	01111110b
	db	00100010b
	db	00100000b
	db	00111000b
	db	00100000b
	db	00100010b
	db	01111110b
;70 - F
	db	27h
	db	01111110b
	db	00100010b
	db	00100000b
	db	00111000b
	db	00100000b
	db	00100000b
	db	01110000b
;71 - G
	db	27h
	db	00011100b
	db	00100010b
	db	01000000b
	db	01000000b
	db	01001110b
	db	00100010b
	db      00011110b
;72 - H
	db	27h
	db	01000010b
	db	01000010b
	db	01000010b
	db	01111110b
	db	01000010b
	db	01000010b
	db	01000010b
;73 - I
	db	27h
	db	00111000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;74 - J
	db	27h
	db	00001110b
	db	00000100b
	db	00000100b
	db	00000100b
	db	00000100b
	db	01000100b
	db	00111000b
;75 - K
	db	27h
	db	01100010b
	db	00100100b
	db	00101000b
	db	00111000b
	db	00100100b
	db	00100010b
	db	01100010b
;76 - L
	db	27h
	db	01110000b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100010b
	db	01111110b
;77 - M
	db	27h
	db	01000010b
	db	01100110b
	db	01011010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
;78 - N
	db	27h
	db	01000010b
	db	01100010b
	db	01010010b
	db	01001010b
	db	01000110b
	db	01000010b
	db	01000010b
;79 - O
	db	27h
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;80 - P
	db	27h
	db	01111100b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100000b
	db	00100000b
	db	01110000b
;81 - Q
	db	28h
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01001010b
	db	01001010b
	db	00111100b
	db	00000110b

;82 - R
	db	27h
	db	01111100b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100100b
	db	00100010b
	db	01100010b
;83 - S
	db	27h
	db	00111100b
	db	01000010b
	db	01000000b
	db	00111100b
	db	00000010b
	db	01000010b
	db	00111100b
;84 - T
	db	27h
	db	11111110b
	db	10010010b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;85 - U
	db	27h
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;86 - V
	db	27h
	db	10000010b
	db	10000010b
	db	10000010b
	db	10000010b
	db	01000100b
	db	00101000b
	db	00010000b
;87 - W
	db	27h
	db	10000010b
	db	10000010b
	db	10000010b
	db	10010010b
	db	10010010b
	db	10101010b
	db	01000100b
;88 - X
	db	27h
	db	01000010b
	db	01000010b
	db	00100100b
	db	00011000b
	db	00100100b
	db	01000010b
	db	01000010b
;89 - Y
	db	27h
	db	10000010b
	db	10000010b
	db	01000100b
	db	00111000b
	db	00010000b
	db	00010000b
	db	00111000b
;90 - Z
	db	27h
	db	01111110b
	db	01000100b
	db	00001000b
	db	00010000b
	db	00100000b
	db	01000010b
	db	01111110b
;91 - [
	db	19h
	db	00011100b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00011100b
;92 - \
;
	db	36h
	db	01000000b
	db	00100000b
	db	00010000b
	db	00001000b
	db	00000100b
	db	00000010b
;93 - ]
	db	19h
	db	00111000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00111000b
;94 - ^
	db	13h
	db	00010000b
	db	00101000b
	db	01000100b
;95 - _
	db	81h
	db	11111111b

;96 - `
	db	13h
	db	00011000b
	db	00010000b
	db	00001000b
;97 - a
	db	45h
	db	00111000b
	db	00000100b
	db	00111100b
	db	01000100b
	db	00111010b
;98 - b
	db	27h
	db	01100000b
	db	00100000b
	db	00111000b
	db	00100100b
	db	00100010b
	db	00100010b
	db	01011100b
;99 - c
	db	45h
	db	00111100b
	db	01000000b
	db	01000000b
	db	01000000b
	db	00111100b
;100 - d
	db	27h
	db	00001100b
	db	00000100b
	db	00011100b
	db	00100100b
	db	01000100b
	db	01000100b
	db	00111010b
;101 - e
	db	45h
	db	00111100b
	db	01000010b
	db	01111110b
	db	01000000b
	db	00111100b
;102 - f
	db	27h
	db	00011000b
	db	00100100b
	db	00100000b
	db	01111000b
	db	00100000b
	db	00100000b
	db	01110000b
;103 - g
	db	46h
	db	00111010b
	db	01000100b
	db	01000100b
	db	00111100b
	db	00000100b
	db	01111000b
;104 - h
	db	27h
	db	01100000b
	db	00100000b
	db	00101100b
	db	00110010b
	db	00100010b
	db	00100010b
	db	01100010b
;105 - i
	db	27h
	db	00010000b
	db	00000000b
	db	00110000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;106 - j
	db	28h
	db	00000100b
	db	00000000b
	db	00001100b
	db	00000100b
	db	00000100b
	db	00000100b
	db	01000100b
	db	00111000b
;107 - k
	db	27h
	db	01100000b
	db	00100000b
	db	00100010b
	db	00100100b
	db	00111000b
	db	00100100b
	db	01100010b
;108 - l
	db	27h
	db	00110000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;109 - m
	db	45h
	db	11101100b
	db	10010010b
	db	10010010b
	db	10010010b
	db	10010010b
;110 - n
	db	45h
	db	01011100b
	db	00100010b
	db	00100010b
	db	00100010b
	db	00100010b
;111 - o
	db	45h
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;112 - p
	db	47h
	db	01011100b
	db	00100010b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100000b
	db	01110000b
;113 - q
	db	47h
	db	00111010b
	db	01000100b
	db	01000100b
	db	01000100b
	db	00111100b
	db	00000100b
	db      00001110b
;114 - r
	db	45h
	db	01001100b
	db	00110010b
	db	00100000b
	db	00100000b
	db	01110000b
;115 - s
	db	45h
	db	00111100b
	db	01000000b
	db	00111100b
	db	00000010b
	db	01111100b
;116 - t
	db	27h
	db	00100000b
	db	00100000b
	db	01110000b
	db	00100000b
	db	00100000b
	db	00100100b
	db	00011000b
;117 - u
	db	45h
	db	01000100b
	db	01000100b
	db	01000100b
	db	01000100b
	db	00111010b
;118 - v
	db	45h
	db	01000100b
	db	01000100b
	db	01000100b
	db	00101000b
	db	00010000b
;119 - w
	db	45h
	db	10000010b
	db	10000010b
	db	10010010b
	db	10101010b
	db	01000100b
 ;120 - x
	db	45h
	db	01000010b
	db	00100100b
	db	00011000b
	db	00100100b
	db	01000010b
;121 - y
	db	47h
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111110b
	db	00000010b
	db	01000010b
	db	00111100b
;122 - z
	db	45h
	db	01111100b
	db	01001000b
	db	00010000b
	db	00100100b
	db	01111100b
;123 - {
	db	19h
	db	00000110b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00110000b
	db	00001000b
	db	00001000b
	db	00001000b
	db	00000110b
;124 - |
	db	28h
	db	00010000b
	db	00010000b
	db	00010000b
	db	00000000b
	db	00000000b
	db	00010000b
	db	00010000b
	db	00010000b
;125 - }
	db	19h
	db	01100000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00001100b
	db	00010000b
	db	00010000b
	db	00010000b
	db	01100000b
;126 - ~
	db	22h
	db	01110010b
	db	10001100b
;127 - nic
	db	10h

;128 -
	db	09h
	db	00100100b
	db	00011000b
	db	00011100b
	db	00100010b
	db	01000000b
	db	01000000b
	db	01000000b
	db	00100010b
	db	00011100b
;129 - 
	db	27h
	db	01000100b
	db	00000000b
	db	01000100b
	db	01000100b
	db	01000100b
	db	01000100b
	db	00111010b
;130 - 
	db	18h
	db	00000100b
	db	00001000b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01111110b
	db	01000000b
	db	00111100b
;131 - 
	db	09h
	db	01001000b
	db	00110000b
	db	00000100b
	db	00000100b
	db	00011100b
	db	00100100b
	db	01000100b
	db	01000100b
	db	00111010b
;132 -
	db	27h
	db	01000100b
	db	00000000b
	db	00111000b
	db	00000100b
	db	00111100b
	db	01000100b
	db	00111010b
;133 - 
	db	09h
	db	00100100b
	db	00011000b
	db	01111000b
	db	00100100b
	db	00100010b
	db	00100010b
	db	00100010b
	db	00100100b
	db	01111000b
;134 - 
	db	09h
	db	00101000b
	db	00010000b
	db	11111110b
	db	10010010b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;135 -
	db	18h
	db	00100100b
	db	00011000b
	db	00000000b
	db	00111100b
	db	01000000b
	db	01000000b
	db	01000000b
	db	00111100b
;136 - 
	db	18h
	db	00100100b
	db	00011000b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01111110b
	db	01000000b
	db	00111100b
;137 -
	db	09h
	db	00100100b
	db	00011000b
	db	01111110b
	db	00100010b
	db	00100000b
	db	00111000b
	db	00100000b
	db	00100010b
	db	01111110b
;138 - 
	db	09h
	db	00000110b
	db	00000010b
	db	01110100b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100010b
	db	01111110b
;139 -
	db	09h
	db	00001000b
	db	00010000b
	db	00111000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;140 -
	db	09h
	db	00010010b
	db	00001100b
	db	00110000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;141 -
	db	09h
	db	00000110b
	db	00000010b
	db	00110100b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;142 -
	db	09h
	db	01000010b
	db	00000000b
	db	00011000b
	db	00100100b
	db	01000010b
	db	01000010b
	db	01111110b
	db	01000010b
	db	01000010b
;143 - 
	db	09h
	db	00000100b
	db	00001000b
	db	00011000b
	db	00100100b
	db	01000010b
	db	01000010b
	db	01111110b
	db	01000010b
	db	01000010b
;144 -
	db	09h
	db	00000100b
	db	00001000b
	db	01111110b
	db	00100010b
	db	00100000b
	db	00111000b
	db	00100000b
	db	00100010b
	db	01111110b
;145 - 
	db	18h
	db	01001000b
	db	00110000b
	db	00000000b
	db	01111100b
	db	01001000b
	db	00010000b
	db	00100100b
	db	01111100b
;146 - 
	db	09h
	db	00100100b
	db	00011000b
	db	01111110b
	db	01000100b
	db	00001000b
	db	00010000b
	db	00100000b
	db	01000010b
	db	01111110b
;147 -
	db	18h
	db	00111100b
	db	01000010b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;148 -
	db	27h
	db	01000010b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;149 -
	db	09h
	db	00000100b
	db	00001000b
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;150 -
	db	18h
	db	00010000b
	db	00101000b
	db	00010000b
	db	01000100b
	db	01000100b
	db	01000100b
	db	01000100b
	db	00111010b
;151 -
	db	09h
	db	00001000b
	db	00010000b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;152 -
	db	1Ah
	db	00001000b
	db	00010000b
	db	00000000b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111110b
	db	00000010b
	db	01000010b
	db	00111100b
;153 -
	db	09h
	db	01000010b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;154 -
	db	09h
	db	01000010b
	db	00000000b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;155 -
	db	09h
	db	00100100b
	db	00011000b
	db	00111100b
	db	01000010b
	db	01000000b
	db	00111100b
	db	00000010b
	db	01000010b
	db	00111100b
;156 - 
	db	27h     
	db	00011000b
	db	00100100b
	db	00100000b
	db	01110000b
	db	00100000b
	db	01110010b
	db	01001100b

;157 -
	db	09h
	db	00001000b
	db	00010000b
	db	10000010b
	db	10000010b
	db	01000100b
	db	00111000b
	db	00010000b
	db	00010000b
	db	00111000b
;158 -
	db	09h
	db	00100100b
	db	00011000b
	db	01111100b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100100b
	db	00100010b
	db	01100010b
;159 -
	db	09h
	db	00010010b
	db	00001100b
	db	00100000b
	db	00100000b
	db	01110000b
	db	00100000b
	db	00100000b
	db	00100100b
	db	00011000b
;160 -
	db	18h
	db	00001000b
	db	00010000b
	db	00000000b
	db	00111000b
	db	00000100b
	db	00111100b
	db	01000100b
	db	00111010b
;161 -
	db	18h
	db	00001000b
	db	00010000b
	db	00000000b
	db	00110000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00111000b
;162 -
	db	18h
	db	00001000b
	db	00010000b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;163 -
	db	18h
	db	00001000b
	db	00010000b
	db	00000000b
	db	01000100b
	db	01000100b
	db	01000100b
	db	01000100b
	db	00111010b
;164 -
	db	18h
	db	00100100b
	db	00011000b
	db	00000000b
	db	01011100b
	db	00100010b
	db	00100010b
	db	00100010b
	db	00100010b
;165 -
	db	09h
	db	00100100b
	db	00011000b
	db	01000010b
	db	01100010b
	db	01010010b
	db	01001010b
	db	01000110b
	db	01000010b
	db	01000010b
;166 -
	db	09h
	db	00011000b
	db	00100100b
	db	01011010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;167 -
	db	09h
	db	00111100b
	db	01000010b
	db	00000000b
	db	00111100b
	db	01000010b
	db	01000010b
	db	01000010b
	db	01000010b
	db	00111100b
;168 -
	db	18h
	db	00100100b
	db	00011000b
	db	00000000b
	db	00111100b
	db	01000000b
	db	00111100b
	db	00000010b
	db	01111100b
;169 -
	db	18h
	db	00100100b
	db	00011000b
	db	00000000b
	db	01001100b
	db	00110010b
	db	00100000b
	db	00100000b
	db	01110000b
;170 -
	db	18h
	db	00001000b
	db	00010000b
	db	00000000b
	db	01001100b
	db	00110010b
	db	00100000b
	db	00100000b
	db	01110000b
;171 -nic
	db	10h
;172 - nic
	db	10h
;173 -
	db	0Ah
	db	00111000b
	db	01000100b
	db	01000000b
	db	00111000b
	db	01000100b
	db	01000100b
	db	00111000b
	db	00000100b
	db	01000100b
	db	00111000b
;174 -
	db	35h
	db	00010010b
	db	00100100b
	db	01001000b
	db	00100100b
	db	00010010b
;175 -
	db	35h
	db	01001000b
	db	00100100b
	db	00010010b
	db	00100100b
	db	01001000b
;
	db	0		;pøíznak posunu
	db	179
;179 -
	db	0Ch
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;180 -
	db	0Ch
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	11110000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;
	db	0		;pøíznak posunu
	db	185
;185 -
	db	0Ch
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	11101000b
	db	00001000b
	db	11101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;186 -
	db	0Ch
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;187 -
	db	57h
	db	11111000b
	db	00001000b
	db	11101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;188 -
	db	08h
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	11101000b
	db	00001000b
	db	11111000b
;189 - nic
	db	10h
;190 - nic
	db	10h
;191 -
	db	66h
	db	11110000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;192 -
	db	07h
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00011111b     
;193 -
	db	07h
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	11111111b
;194 -
	db	66h
	db	11111111b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;195 -
	db	0Ch
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00011111b     
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;196 -
	db	61h
	db	11111111b
;197 -
	db	0Ch
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	11111111b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;198 - nic
	db	10h
;199 - nic
	db	10h
;200 -
	db	08h
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101111b
	db	00100000b
	db	00111111b
;201 -
	db	57h
	db	00111111b     
	db	00100000b
	db	00101111b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;202 -
	db	08h
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	11101111b
	db	00000000b
	db	11111111b
;203 -
	db	57h
	db	11111111b
	db	00000000b
	db	11101111b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;204 -
	db	0Ch
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101111b
	db	00100000b
	db	00101111b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;205 -
	db	53h
	db	11111111b
	db	00000000b
	db	11111111b
;206 -
	db	0Ch
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
	db	11101111b
	db	00000000b
	db	11101111b
	db	00101000b
	db	00101000b
	db	00101000b
	db	00101000b
;
	db	0		;pøíznak posunu
	db	217
 ;217 -
	db	07h
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	11110000b
 ;218 -
	db	66h
	db	00011111b     
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
	db	00010000b
;
	db	0		;pøíznak posunu
	db	224
 ;224 -
	db	45h
	db	01100010b
	db	10010100b
	db	10001000b
	db	10010100b
	db	01100010b
 ;225 -
	db	27h
	db	00011100b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100010b
	db	00110010b
	db	01101100b
 ;226 -
	db	27h
	db	01111110b
	db	00100010b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100000b
	db	01110000b
 ;227 -
	db	36h
	db	00000010b
	db	01111100b
	db	10100100b
	db	00100100b
	db	00100100b
	db	01100110b
 ;228 -
	db	27h
	db	11111110b
	db	01000010b
	db	00100000b
	db	00010000b
	db	00100000b
	db	01000010b
	db	11111110b
 ;229 -
	db	45h
	db	00111110b
	db	01000100b
	db	01000100b
	db	01000100b
	db	00111000b
 ;230 -
	db	46h
	db	01000100b
	db	01000100b
	db	01000100b
	db	01111100b
	db	01000010b
	db	01000000b
 ;231 -
	db	36h
	db	00000010b
	db	01111100b
	db	10010000b
	db	00010000b
	db	00010000b
	db	00001000b
 ;232 -
	db	27h
	db	01111100b
	db	00010000b
	db	00111000b
	db	01010100b
	db	00111000b
	db	00010000b
	db	01111100b
 ;233 -
	db	27h
	db	00011000b
	db	00100100b
	db	01000010b
	db	01011010b
	db	01000010b
	db	00100100b
	db	00011000b
 ;234 -
	db	27h
	db	00111000b
	db	01000100b
	db	10000010b
	db	10000010b
	db	01000100b
	db	00101000b
	db	11101110b
 ;235 -
	db	27h
	db	00110010b
	db	01001100b
	db	00100000b
	db	00111000b
	db	01000100b
	db	01000100b
	db	00111000b
 ;236 -
	db	45h
	db	01101100b
	db	10010010b
	db	10010010b
	db	10010010b
	db	01101100b
;
	db	0		;pøíznak posunu
	db	246
 ;246 -
	db	35h
	db	00010000b
	db	00000000b
	db	01111100b
	db	00000000b
	db	00010000b
 ;247 -
	db	35h
	db	00110010b
	db	01001100b
	db	00000000b
	db	00110010b
	db	01001100b
 ;248 -
	db	14h
	db	00110000b
	db	01001000b
	db	01001000b
	db	00110000b
 ;249 -
	db	09h
	db	00010010b
	db	00001100b
	db	01110000b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100000b
	db	00100010b
	db	01111110b
 ;250 -
	db	09h
	db	00001000b
	db	00010000b
	db	01111100b
	db	00100010b
	db	00100010b
	db	00111100b
	db	00100100b
	db	00100010b
	db	01100010b
 ;251 - nic
	db	10h
 ;252 -
	db	14h
	db	10110000b
	db	01001000b
	db	01001000b
	db	01001000b
 ;253 -
	db	14h
	db	01110000b
	db	00010000b
	db	00100000b
	db	01110000b
 ;254 -
	db	12h
	db	00001000b
	db	00010000b
 ;255 -
	db	12h
	db	00100100b
	db	00011000b
 ;
	db	0FFh		;koncová znaèka
;konec pøedloh fontu

;ještì zbylo 6 bajtù volno :)
;----------------------------
	db	0FFh,0FFh,0FFh,0FFh,0FFh,0FFh
		
;-----------------------------------------------------------------------------
;
;	Include External Tables and Procedures
;
;         ***********************************
;         *                                 *
;         *       Z256 Microcomputer        *
;         *                                 *
;         ***********************************
;         *                                 *
;         *   EXTERNAL Procedure Ver 3.0    *
;         *                                 *
;         *   Copyright (C) 1988 by Petr    *
;         *                                 *
;         *     Last UpDate: 17.07.1988     *
;         *                                 *
;         ***********************************
;
;	ORG	CHGEPR+2048-EXBLCK	;mìlo by být 45F0
;
;****************************************************************
;*                                                              *
;*              TABLES FOR MOTOROLA 6845 CRT                    *
;*                                                              *
;****************************************************************
;
;	CRTCHT - table for programming of Motorola 6845 CRT controller
;		 to Character Mode
;
CRTCHT:	DEFB	99		;R0  - horizontal total-1
	DEFB	XMAX		;R1  - horizontal displayed
	DEFB	83		;R2  - hor. sync position
	DEFB	10		;R3  - hor. sync width
	DEFB	25		;R4  - vertical total-1
	DEFB	2		;R5  - vertical adjust
	DEFB	YMAX		;R6  - vertical displayed
	DEFB	24		;R7  - vert. sync position
	DEFB	0		;R8  - interlace and skew
	DEFB	11		;R9  - maximum scan line-1
	DEFB	106		;R10 - cursor start
	DEFB	11		;R11 - cursor end
;
;-----------------------------------------------------------------------------
;
;	CRTGRT - table for programming of Motorola 6845 CRT controller
;		 to Graphic Mode
;
CRTGRT:	DEFB	99		;R0  - horizontal total-1
	DEFB	XMAX		;R1  - horizontal displayed
	DEFB	83		;R2  - hor. sync position
	DEFB	10		;R3  - hor. sync width
	DEFB	18		;R4  - vertical total-1
	DEFB	10		;R5  - vertical adjust
	DEFB	18		;R6  - vertical displayed
	DEFB	18		;R7  - vert. sync position
	DEFB	0		;R8  - interlace and skew
	DEFB	15		;R9  - maximum scan line-1
	DEFB	32		;R10 - cursor disable
	DEFB	00		;R11 - cursor end
;
;
;****************************************************************
;*                                                              *
;*          CRT VARIABLES AND PROCEDURES ALLOCATED IN RAM       *
;*                                                              *
;****************************************************************
;
CRTARR	EQU	$		;begin of CRT area in EPROM
;
	PHASE	VWRKAR		;begin of video work area
;
;-----------------------------------------------------------------------------
;
;	CRT Variables
;
X:	DEFB	0		;X (column) cursor address
Y:	DEFB	0		;Y (row) cursor address
;
MA:	DEFW	0		;Motorola cursor address
VMOFF:	DEFW	0		;Motorola video page offset
;
ACTFNT:	DEFB	40H		;MSB address of selected font
;
FCOLOR:	DEFB	'2'		;foreground color
BCOLOR:	DEFB	'0'		;background color
INVSTS:	DEFB	0		;invert status 0 or 3
;
ROUTER:	DEFW	SELECT		;router for CONOUT selector now dummy
;
STACKP:				;Stack Pointer save area for Floppy proc.
WINDWR:	DEFW	DUMMY		;Move Window work area
;
;-----------------------------------------------------------------------------
;
; Floppy variables and tables
;
MOFFDL:	DEFB	DUMMY/256	;motor off delay in 100ms
MONDL:	DEFB	50		;motor on delay in 10ms (O.5sec for Boot)
LSTSEC:	DEFB	1		;number of last sector on track
;
;       Driver Mask
;
;   MotSw (bit 7) = 0 ... MotOn is not switching
;                 = 1 ... MotOn is switching according to MOnDl & MOffDl
;
;         (bit 6) =	. .. not used
;
;   InUse (bit 5) = 0 ... set out In Use to inactive state (log. 1)
;                 = 1 ... set  out In Use to active state (log. 0)
;
;   MotOn (bit 4) = 0 ... set out Motor On to inactive state (log. 1)
;                 = 1 ... set out Motor On to active state (log. 0)
;
;   8"    (bit 3) = 0 ... set drive 5 inch
;                 = 1 ... set drive 8 inch
;
;   SDens (bit 2) = 0 ... set double density
;                 = 1 ... set single density
;
;   Drv (bit 1,0) = n ... set drive n (for n = 0 to 3)
;
DRVMSK:	DEFB	EIGHT+SDENS+DRVR0
;
;       Driver Characteristic
;
;   SideS (bit 7) = 0 ... one side medium or drive
;                 = 1 ... two side medium or drive
;
;   DbStp (bit 6) = 0 ... one step per track (medium & drive the same tpi)
;                 = 1 ... two step per track (48tpi medium & 96tpi drive)
;
;   DTp (bit 5,4) = 00 .. floppy disk
;                 = 01 .. hard disk
;                 = 10 .. RAM disk
;                 = 11 .. ROM disk
;
;   StpCl (bit 3) = 0 ... no change FDC clock for stepping
;                 = 1 ... invert FDC clock for stepping
;
;         (bit 2) =   ... not used
;
;   SRt (bit 1,0) = 00 .. step rate  3 or  6 ms
;	=	01 .. step rate  6 or 12 ms
;	=	10 .. step rate 10 or 20 ms
;	=	11 .. step rate 15 or 30 ms
;
BOOTTB:				;initialized table for Boot
;
DRVCHR:	DEFB	03H		;driver charact. - step rate 15 or 30 ms
;
FLSIDE:	DEFB	0		;side address (0 or Side1)
;
LTRKTB:	DEFB	DUMMY/256	;last track for drive 0
	DEFB	DUMMY/256	;last track for drive 1
	DEFB	DUMMY/256	;last track for drive 2
	DEFB	DUMMY/256	;last track for drive 3
;
;****************************************************************
;*                                                              *
;*                MESSAGES FOR FLOPPY PROCEDURES                *
;*                                                              *
;****************************************************************
;
NSYSMS:	DEFB	CRLF,"Non System Disk !",0
;
BTERMS:	DEFB	CRLF,"Boot Error !",0
;
;
;****************************************************************
;*                                                              *
;*                    CONOUT SUBPROCEDURES                      *
;*                                                              *
;****************************************************************
;
;	PROCTB - this table contents address of procedures for
;		 character writing and clear to end of line
;
PROCTB:	DEFW	WRSP		;write space
	DEFW	WRINSP		;write invert space
	DEFW	COPCHA		;write character
	DEFW	CINCHA		;write invert character
;
;-----------------------------------------------------------------------------
;
;	BLPRTB - this table contents address of procedures for
;		 clear screen
;
BLPRTB:	DEFW	CLBLPG		;clear page with null
	DEFW	CLINPG		;clear page with invert null
;
;-----------------------------------------------------------------------------
;
;	COPCHA - this procedure copies character from char.gen. to
;		 video memory
;
;	Input:	A  - page select mask
;		DE - source address in generator
;		HL - destination address in video memory
;
COPCHA:	OUT	(BANK),A	;select page
	OUT	(SBOARD),A	;select video board
;
	REPT	YPIXEL-1
;
	LD	A,(DE)		;get byte from generator
	LD	(HL),A		;set it to video mem.
	INC	D		;next generator line
	INC	H		;next video line
;
	ENDM
;
	LD	A,(DE)		;get last byte from generator
	LD	(HL),A		;set it to video mem.
	OUT	(MBOARD),A	;select RAM board
	XOR	A		;page 0 select
	OUT	(BANK),A	;select page
	RET
;
;-----------------------------------------------------------------------------
;
;	CINCHA - this procedure copies invert character from char.gen. to
;		 video memory
;
;	Input:	A  - page select mask
;		DE - source address in generator
;		HL - destination address in video memory
;
CINCHA:	OUT	(BANK),A	;select page
	OUT	(SBOARD),A	;select video board
;
	REPT	YPIXEL-1
;
	LD	A,(DE)		;get byte from generator
	CPL			;invert it
	LD	(HL),A		;set it to video mem.
	INC	D		;next generator line
	INC	H		;next video line
;
	ENDM
;
	LD	A,(DE)		;get Iast byte from generator
	CPL	;invert 	it
	LD	(HL),A		;set it	to video mem.
	OUT	(MBOARD),A	;select RAM board
	XOR	A		;page 0 select
	OUT	(BANK),A	;select page
	RET
;
;-----------------------------------------------------------------------------
;
;	WRSP - this procedure writes null to video memory
;
; Input:  A  - page select mask
;         HL - destination address in video memory
;
WRSP:	OUT	(BANK),A	;select page
	XOR	A		;set null character
WRSP1:	OUT	(SBOARD),A	;select video board
;
	REPT	YPIXEL-1
;
	LD	(HL),A		;fill one line with null
	INC	H		;next line
;
	ENDM
;
	LD	(HL),A		;fill last line with null
	OUT	(MBOARD),A	;select	RAM board
	XOR	A		;page 0	select
	OUT	(BANK),A	;select	page
	RET
;
;-----------------------------------------------------------------------------
;
;	WRINSP - this procedure writes invert null to video memory
;
;	Input:	A  - page select mask
;		HL - destination address in video memory
;
WRINSP:	OUT	(BANK),A	;select page
	LD	A,0FFH		;set invert null character
	JP	WRSP1		;fill character with invert null
;
;-----------------------------------------------------------------------------
;
;	CLINPG - this procedure clears page with invert null chars
;
;	Input:	A  - page select mask
;
CLINPG:	LD	D,0FFH		;set invert null char.
	DEFB	01EH		;instr. code of LD E,data
;
;-----------------------------------------------------------------------------
;
;	CLBLPG - this procedure clears page with null chars
;
;	Input:	A  - page select mask
;
CLBLPG:	LD	D,00H		;set null char.
	LD	E,D		;DE - pattern for clearing
	OUT	(BANK),A	;select page
	XOR	A		;clear A
	LD	B,A		;clear B - 256*DJNZ
	LD	H,A		;clear HL
	LD	L,A		;for save stack pointer
	ADD	HL,SP		;save stack
	LD	SP,0000H	;set stack on top of memory
	LD	C,08H		;set block counter
	OUT	(SBOARD),A	;select video board
;
CLBLP1:	REPT	8		;for max. speed 8 times PUSH
				;for one loop
	PUSH	DE
;
	ENDM
;
	DJNZ	CLBLP1		;fills 16*256=4k byte of video mem.
	DEC	C		;if next block
	JP	NZ,CLBLP1	;then repeat
	OUT	(MBOARD),A	;else select RAM board
	OUT	(BANK),A	;select page 0
	LD	SP,HL		;restore stack pointer
	RET
;
;****************************************************************
;*                                                              *
;*                          CRT Procedures                      *
;*                                                              *
;****************************************************************
;
;	MOVECH - this procedure moves the character from char. generator
;		 to video RAM
;
MOVECH:	LD	A,WRMP1+RDMP1	;set page 1 for read/write
MOVCH1:	CALL	DUMMY		;COPCHA,CINCHA,WRSP or WRINSP
	LD	H,C		;restore destination address
	EX	AF,AF'		;get MSB of char. gen. address
	LD	D,A		;restore source address
	LD	A,WRMP2+RDMP1	;set page 2 for write page 1 for read
MOVCH2:	JP	DUMMY		;COPCHA,CINCHA,WRSP or WRINSP
;
;-----------------------------------------------------------------------------
;
;	WRBLNK - this procedure writes a blank character to video RAM
;
WRBLNK:	LD	A,WRMP1+RDMP1	;set page 1 for read/write
	LD	H,C		;set MSB of PA for writing space
WRBL1:	CALL	DUMMY		;WRSP or WRINSP
	LD	A,WRMP2+RDMP1	;set page 2 for write page 1 for read
	LD	H,C		;set MSB of PA for writing space
WRBL2:	JP	DUMMY		;WRSP or WRINSP
;
;-----------------------------------------------------------------------------
;
;	CLEARS - this procedure clear screen
;
CLEARS:	LD	A,WRMP1+RDMP1	;set page 1 for read/write
CLRS1:	CALL	DUMMY	;CLBLPG or CLINPG
	LD	A,WRMP2+RDMP1	;set page 2 for write page 1 for read
CLRS2:	JP	DUMMY	;CLBLPG or CLINPG
;
;-----------------------------------------------------------------------------
;
;	COPWIN - this procedure copies window to/from data block
;
;	Input:	A  - page select
;		C  - window width
;		DE - start address of data block
;		HL - PA address of upper left hand corner
;		DE'- window hight
;
;	Output: DE - first free byte in data area
;
COPWIN:	OUT	(BANK),A	;select page
	OUT	(SBOARD),A	;select video board
COPWN1:	EXX			;swap to line number
	LD	A,D		;if all lines
	OR	E		;transferred
	DEC	DE		;prepare next free byte
	EXX			;swap to copy addresses
	JR	Z,COPWN4	;then goto end
;
	LD	B,C		;set window width to counter
	LD	(WINDWR),HL	;save start addr. of window line
COPWN2:	LD	A,(HL)		;get byte from window
	LD	(DE),A		;save byte to data block
	INC	DE		;next byte in data block
	INC	L		;next byte in window
	JP	NZ,COPWN3	;if no bound in window then skip
	LD	A,H		;else add bound offset
	ADD	A,10H		;to MSB of PA
	LD	H,A		;and save new PA
COPWN3:	DJNZ	COPWN2		;repeat for one line of window
	LD	HL,(WINDWR)	 ;restore start addr. of window line
	INC	H		;goto next line of window
	LD	A,H		;if line number
	AND	0FH		;not overflow
	JR	NZ,COPWN1	;then skip
	LD	A,L		;else add line length
	ADD	A,XMAX		;to LSB address
	LD	L,A		;and save	it
	JP	C,COPWN1 	;if overflow then skip - already added
	LD	A,H		;else subtract
	SUB	10H		;overflow from MSB
	LD	H,A		;and save O.K. PA
	JP	COPWN1
;
COPWN4:	OUT	(MBOARD),A	;select RAM board
	XOR	A		;select page 0
	OUT	(BANK),A	;
	RET
;
;-----------------------------------------------------------------------------
;
;	DRAWDT - this is the begin of Draw Dot procedure
;
DRAWDT:
;
CRTLEN	EQU	$-VWRKAR 	;length of CRT area in EPROM
;
;
;****************************************************************
;*                                                              *
;*                    TABLES AND MESSAGESS                      *
;*                                                              *
;****************************************************************
;
;	PTABLE - table for Input/Output devices initialization
;
PTABLE:
;
DMA1T:	DEFB	DMA1L		;length of DMA table
	DEFB	DMA1SC		;DMA status/command reg. address
	DEFB	0C3H		;DMA reset command
;
DMA1L	EQU	$-DMA1T-2	;length of table
;
CTC10T:	DEFB	CTC10L		;length of CTC10 table
	DEFB	CTC10		;CTC10 address
	DEFB	CTC1VT & 255	;low byte of CTC1 vector tab. addr.
;
CTC10L	EQU	$-CTC10T-2	;length of table
;
CTC11T:	DEFB	CTC11L		;length of CTC11 table
	DEFB	CTC11		;CTC11 address
	DEFB	CTCRES+CTCCNT+CTCLTC ;mode of CTC11
	DEFB	19200/S1BBR 	;set baud rate for channel 1B
;
CTC11L	EQU	$-CTC11T-2	;length of table
;
CTC12T:	DEFB	CTC12L		;length of CTC12 table
	DEFB	CTC12		;CTC12 address
	DEFB	CTCRES+CTC256+CTCLTC	;mode of CTC12
	DEFB	(PERD1*SYSCLK*125)/32	;time constant for 10ms
;
CTC12L	EQU	$-CTC12T-2	;length of table
;
SIO1BT:	DEFB	SIO1BL		;length of SIO1B table
	DEFB	SIO1BC		;SIO1B command reg. address
	DEFB	SIORES		;channel reset
	DEFB	SIOR4		;reg.4
	DEFB	SIOSB1+SIOC16	;no parity, 1 stop bit
				;16x Clock Rate = 9600 baud
	DEFB	SIOR3
	DEFB	SIOREE+SIOAUE+SIORC8
				;receive enable, auto enables
				;8bit/receive character
	DEFB	SIOR5
	DEFB	S1CW5		;transmit enable, 8bit/transmit char.
				;RTS active
	DEFB	SIOR1
	DEFB	0
;
SIO1BL	EQU	$-SIO1BT-2	;length of table
;
PIO1AT:	DEFB	PIO1AL		;length of PIO1A table
	DEFB	PIO1AC		;PIO1A command reg. address
	DEFB	PIOCTR		;PIO Control Mode
	DEFB	01111111B	;log=1 - input bit
	DEFB	PIOIDI		;disable interrupt
;
PIO1AL	EQU	$-PIO1AT-2	;length of table
;
PI1ADT:	DEFB	PI1ADL		;length of PI1AD table
	DEFB	PIO1AD		;PIO1A data reg. address
	DEFB	11111111B	;set STROBE to inactive state
;
PI1ADL	EQU	$-PI1ADT-2	;length of table
;
PIO1BT:	DEFB	PIO1BL		;length of PIO1B table
	DEFB	PIO1BC		;PIO1B	;command reg. address
	DEFB	PIOOUT		;PIO Output Mode
	DEFB	PIOIDI		;disable interrupt
;
PIO1BL	EQU	$-PIO1BT-2	;length of table
;
PIO2BT:	DEFB	PIO2BL		;length of PIO2B table
	DEFB	PIO2BC		;PIO2B command reg. address
	DEFB	PIO2BV & 255	;PIO2 vector interrupt table
	DEFB	PIOCTR		;PIO Control Mode
	DEFB	11000000B	;log.1 - input bit
	DEFB	PIOIOH		;interrupt on high level
	DEFB	01111111B	;bit 7 - DRQ monitored for intr.
;
PIO2BL	EQU	$-PIO2BT-2 	 ;length of table
;
PI2BDT:	DEFB	PI2BDL		;length of PI2BD table
	DEFB	PIO2BD		;PIO2B data reg. address
	DEFB	00001100B	;8",Single Density,Driver 0
;
PI2BDL	EQU	$-PI2BDT-2	;length of table
;
	DEFB	0		;end of PTABLE for PINIT procedure
;
;****************************************************************
;*                                                              *
;*                      SYSTEM REPORT MESSAGE                   *
;*                                                              *
;****************************************************************
;
SYSMS1:	DEFB	ESC,GOXY,'!/',ESC,PONECH,0
	DEFB	' 256  Microcomputer  ROM System'
	DEFB	ESC,GOXY,'%/(c) ',80H,'VUT Praha by J.',9BH
	DEFB	'. P.K. J.F.',CRLF,LF,0
	DEFB	'(c) P.Kotek',0A7h,09
;
;=============================================================================
;
	DEPHASE
;
STOPE2	EQU	$
;
EDTLEN	EQU	$-CRTARR-13	;length of CRT area in EPROM
				;bez (c)P.Kotek
;
;-----------------------------------------------------------------------------
;

;=============================================================================
;
;----------------------------------------------------------------------------- 
;
	END
