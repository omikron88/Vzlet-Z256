; Disassembly of the file "\Z256\z256 FloppyImages\01-boot.bin"
; 
; 
f000 c360f0    jp      0f060h		;poskoè


f003 332e3039  db      "3.09"

f007 4a
f008 64
f009 32
f00a 09
f00b 90
f00c 8c        adc     a,h


;pùvodnì F00D
;------------
ffa0 ff        rst     38h
ffa1 ff        rst     38h

ffa2 ff        db      0FFh		;konc. znaèka
ffa3 05        db      05		;E - poèet I/O sektorù
ffa4 01        db      01		;D - poèáteèní sektor 
ffa5 08        db      08		;E - poèet I/O sektorù
ffa6 02        db      02		;D - poèáteèní sektor

ffa7 00e4      dw      0E400h		;odkud bude CP/M

;smyèka
ffa9 d9        exx			;prohoï registry
ffaa 2c
ffab e5        push    hl
ffac cd1500    call    0015h		;vystav hlavu na poadovanou stopu
ffaf e1        pop     hl
ffb0 d9        exx			;obnov registry
ffb1 e3        ex      (sp),hl		;ulo HL a vyzvedni ukazatel
;vstup zde
ffb2 56        ld      d,(hl)		;vyzvedni pèáteèní sektor
ffb3 14        inc     d		;je to FF?
ffb4 ca00fa    jp      z,0fa00h		;pokud ano, BIOS - ColdBoot
ffb7 15        dec     d		;vra zpìt
ffb8 2b        dec     hl		;o jedno ní
ffb9 5e        ld      e,(hl)		;vyzvedni poèet
ffba 2b        dec     hl		;a ještì ní
ffbb e3        ex      (sp),hl		;ulo na zásobník a vyzvedni hodnotu z (FFA7) - E400
ffbc 3e33      ld      a,33h		;0 0 11 00 11 - kód a ètení z stránky 3, zápis do stránky 0
ffbe 08        ex      af,af'		;do A´
ffbf 01a298    ld      bc,98a2h		;B=98 - multisektorové ètení, C= INI instrukce
ffc2 cd2100    call    0021h		;pøímı pøístup na Floppy IO
ffc5 a7        and     a		;jakı je vısledek?
ffc6 28e1      jr      z,0ffa9h		;kdy je vše OK opakuj
ffc8 e1        pop     hl		;zahoï hodnotu ze zásobníku
ffc9 21deff    ld      hl,0ffdeh	;text CRLF+"Loader Read Error !"
ffcc cd0f00    call    000fh		;vıpis textu zakonèeného 0 z (HL)
ffcf cd1200    call    0012h		;hlava na stopu 0

;start 3
;-------
ffd2 2aa7ff    ld      hl,(0ffa7h)	;vyzvedni hodnotu (E400)
ffd5 e5        push    hl		;na zásobník
ffd6 2e00      ld      l,00h		;vynuluj L - stopa 0
ffd8 d9        exx			;prohoï registry
ffd9 21a6ff    ld      hl,0ffa6h	;promìnná
ffdc 18d4      jr      0ffb2h		;a skoè do smyèky

;text CRLF+"Loader Read Error !"
;-------------------------------
ffde 1f        db      1Fh		;CRLF
ffdf 4c6f6164  db      "Loader Read Error !",0
ffe3 65722052
ffe7 65616420
ffeb 4572726f
ffef 72202100     

;start 2
;-------
f060 310000    ld      sp,0000h		;zásobník na konec RAM
f063 dbd5      in      a,(0d5h)		;FDC PIO kanál B Data         
f065 e603      and     03h		;pouze dolní 2 bity
f067 210bf0    ld      hl,0f00bh	;
f06a b6        or      (hl)		;pøidej naètené bity
f06b 77        ld      (hl),a		;a vlo zpìt
f06c 23        inc     hl		;f00c
f06d cd1e00    call    001eh		;vyber disk
f070 210df0    ld      hl,0f00dh	;odkud
f073 11a0ff    ld      de,0ffa0h	;kam
f076 015300    ld      bc,0053h		;kolik
f079 edb0      ldir			;pøenes
f07b c3d2ff    jp      0ffd2h		;a proveï

;volno
;-----
f07e-f1fe 00   ds      XX x 00h     
f1ff 9f        sbc     a,a
