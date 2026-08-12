; Disassembly of the file "\Z256\z256 FloppyImages\01-BIOS.bin"
; 
; 
fa00 c33cfd    jp      0fd3ch		;Coldboot
fa03 c34efa    jp      0fa4eh		;tepl˝ start (Warmboot)
fa06 c3d9fa    jp      0fad9h		;CONST  - stav kl·vesnice
fa09 c3e1fa    jp      0fae1h		;CONIN  - vstup znaku z kl·vesnice
fa0c c31afb    jp      0fb1ah		;CONOUT - v˝stup znaku na obrazovku
fa0f c320fb    jp      0fb20h		;LIST   - v˝stup znaku na tisk·rnu
fa12 c32cfb    jp      0fb2ch		;PUNCH  - v˝stup znaku na dÏrovaË
fa15 c332fb    jp      0fb32h		;READER - vstup znaku z dÏrnÈ p·sky
fa18 c37efb    jp      0fb7eh 		;HOME   - nastavenÌ ËÌsla stopy na 0
fa1b c35afb    jp      0fb5ah		;SELDSK - v˝bÏr jednotky
fa1e c386fb    jp      0fb86h		;SETTRK - nastavenÌ stopy
fa21 c38bfb    jp      0fb8bh		;SETSEC - nastavenÌ sektoru
fa24 c39bfb    jp      0fb9bh		;SETDMA
fa27 c3a4fb    jp      0fba4h		;READ   - ËtenÌ sektoru
fa2a c3a0fb    jp      0fba0h		;WRITE  - z·pis sektoru
fa2d c326fb    jp      0fb26h		;LISTST - stav tisk·rny
fa30 c390fb    jp      0fb90h		;SECTRAN - p¯eklad adresy sektoru
fa33 c336fb    jp      0fb36h		;volej adresu IX v eprom CPU desky
fa36 c3ffff    jp      0ffffh		;zde sk·Ëe NMI


fa39 04        db      4		;max drive (jednotky 0-4)
fa3a 00ed      dw      0ED00h		;Buffer stopy (9x512 bajt˘) ve str·nce 3 - 4608 bajt˘ (1200h)
					;buffer pro diskovÈ operace
					;od FF00h jsou rutiny pro diskovÈ operace a od FF80 vektory p¯eruöenÌ (pro 3. str·nku)

fa3c cb        db      0cbh
fa3d fe25      cp      25h

fa3f ff        db      255		;p¯Ìznak konce   
fa40 0c        inc     c
fa41 00        db      0		;
fa42 00        db      0		;
fa43 00        db      0		;
fa44 00        db      0		;
fa45 00        db      0		;
fa46 00        db      0		;

fa47 0000      dw      0		;
fa49 00        nop     
fa4a 00        nop     
fa4b 00        nop     
fa4c 00        nop     
fa4d 20        db      20h		;p¯Ìznak systÈm ve str·nce 3 (pro 20h je)

;Warmboot
;--------
fa4e 310001    ld      sp,0100h		;z·sobnÌk do pracovnÌ oblasti CP/M
fa51 216cfc    ld      hl,0fc6ch	;diskovÈ p¯Ìznaky
fa54 cd15fc    call    0fc15h		;zruö diskovÈ p¯Ìznaky (na HL) 3,4 a pokud je 2 zapiö buffer
fa57 214dfa    ld      hl,0fa4dh	;adresa p¯Ìznaku (je tam 32 po startu)
fa5a 7e        ld      a,(hl)		;vyzvedni
fa5b fe20      cp      20h		;je ve str·nce 3 CP/M ?
fa5d 2841      jr      z,0faa0h		;poskoË pokud ano na jeho start
fa5f dd211e00  ld      ix,001eh		;SDRIVE - vyber diskovou jednotu
fa63 cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fa66 dd211200  ld      ix,0012h		;RESTOR - n·vrat na stopu 0
fa6a cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fa6d 2146fa    ld      hl,0fa46h	;zaË·tek tabulky
fa70 e5        push    hl		;na z·sobnÌk
fa71 2e00      ld      l,00h		;HL' bude FA00
fa73 d9        exx			;prohoÔ registry     
fa74 2a47fa    ld      hl,(0fa47h)	;zde po startu 0
;smyËka
fa77 c1        pop     bc		;BC=FA46/FA44/FA42....
fa78 0a        ld      a,(bc)		;vyzvedni (zde 0)
fa79 57        ld      d,a		;dej do D
fa7a 3c        inc     a		;bylo to 255?
fa7b 283b      jr      z,0fab8h		;poskoË pokud ano
fa7d 0b        dec     bc		;BC=FA45
fa7e 0a        ld      a,(bc)		;vyzvedni
fa7f 5f        ld      e,a		;do E
fa80 0b        dec     bc		;BC=FA44
fa81 c5        push    bc		;uloû
fa82 01a298    ld      bc,98a2h		;B p¯Ìkaz, C=instr. INI
fa85 3e33      ld      a,33h
fa87 08        ex      af,af'
fa88 dd212100  ld      ix,0021h		;FLOPIO - p¯Ìm˝ vstup/v˝stup na floppy
fa8c cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fa8f a7        and     a		;v˝sledek operace
fa90 20bc      jr      nz,0fa4eh	;p¯i chybÏ opakuj Warmboot
fa92 d9        exx			;prohoÔ     
fa93 2c        inc     l		;HL=FA01
fa94 e5        push    hl		;uschovej
fa95 dd211500  ld      ix,0015h		;SEEKTR - Hlava na poûadovanou stopu
fa99 cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fa9c e1        pop     hl		;obnov
fa9d d9        exx			;prohoÔ     
fa9e 18d7      jr      0fa77h		;a opakuj

;nahrej CP/M podle (FA3A) ze str·nky 3 a spusù ho
;------------------------------------------------
faa0 01fc80    ld      bc,80fch		;port FC (deska CPU str·nkovacÌ registr), hodnota 1 0 00 00 00
faa3 d9        exx			;z·loûnÌ registry
faa4 2a3afa    ld      hl,(0fa3ah)	;vyzvedni odkud (ED00) ve str·nce 3 - pod bufferem stopy 
faa7 2b        dec     hl		;sniû (je uloûeno na D700-ECFF)
faa8 11fff9    ld      de,0f9ffh	;kam -> E400-F9FF (1600h bajt˘)
faab 010016    ld      bc,1600h		;kolik (dÈlka CCP+BDOS)
faae 3e83      ld      a,83h		;1 0 00 00 11 - ze str·nky 3 do str·nky 0
fab0 f3        di			;z·kaz p¯eruöenÌ      
fab1 d3fc      out     (0fch),a		;deska CPU str·nkovacÌ registr
fab3 edb8      lddr			;p¯enes    
fab5 d9        exx			;registry zpÏt
fab6 ed41      out     (c),b		;p¯epni ram na zapnutÈ str·nkov·nÌ a pr·ci ve str·nce 00 

;inicializuj skoky a start CCP
;-----------------------------
fab8 21d1fa    ld      hl,0fad1h	;odkud
fabb 110000    ld      de,0000h		;kam
fabe 010800    ld      bc,0008h		;dÈlka
fac1 edb0      ldir			;p¯enes - inicializace skok˘ CP/M
fac3 018000    ld      bc,0080h		;adresa pracovnÌ oblasti
fac6 cd9bfb    call    0fb9bh		;SETDMA
fac9 fb        ei			;povol p¯eruöenÌ
faca 3ad5fa    ld      a,(0fad5h)	;ËÌslo uûivatele (0)
facd 4f        ld      c,a		;do C
face c300e4    jp      0e400h		;start CCP

;kopÌruje se na 0000 - prvnÌch 8 bajt˘ pamÏti
;--------------------------------------------
fad1-0000 c303fa    jp      0fa03h		;warmboot
fad4-0003 00        db      0
fad5-0004 00        db      0			;zde i aktu·lnÌ jednotka (FAD5h)
fad6-0005 c306ec    jp      0ec06h		;BDOS

;CONST  - stav kl·vesnice
;------------------------
fad9 3a6afc    ld      a,(0fc6ah)
fadc a7        and     a
fadd c8        ret     z
fade 3eff      ld      a,0ffh
fae0 c9        ret     

;CONIN  - vstup znaku z kl·vesnice
;---------------------------------
fae1 216afc    ld      hl,0fc6ah
fae4 7e        ld      a,(hl)
fae5 a7        and     a
fae6 28f9      jr      z,0fae1h
fae8 f3        di			;zakaû p¯eruöenÌ
fae9 7e        ld      a,(hl)
faea 3d        dec     a
faeb 77        ld      (hl),a
faec 2b        dec     hl
faed 7e        ld      a,(hl)
faee 3c        inc     a
faef e60f      and     0fh
faf1 77        ld      (hl),a
faf2 2125ff    ld      hl,0ff25h
faf5 85        add     a,l
faf6 6f        ld      l,a
faf7 7e        ld      a,(hl)
faf8 fb        ei			;povol p¯eruöenÌ
faf9 c9        ret			;a n·vrat

;rutina p¯eruöenÌ IM2 vektor 8A - ËtenÌ dat z FDC-PIO-A do bufferu
; = zpracov·nnÌ znaku z kl·vesnice
;-----------------------------------------------------------------
fafa f5        push    af		;uschovej registry
fafb e5        push    hl
fafc 216afc    ld      hl,0fc6ah	;
faff 7e        ld      a,(hl)
fb00 fe10      cp      10h		;16 a vÌc?
fb02 3011      jr      nc,0fb15h	;poskoË pokud ano
fb04 3c        inc     a		;zvyö
fb05 77        ld      (hl),a		;uloû zpÏt
fb06 23        inc     hl		;posuÚ se na FC6B
fb07 7e        ld      a,(hl)		;vyzvedni hodnotu
fb08 3c        inc     a		;zvyö
fb09 e60f      and     0fh		;pouze dolnÌ 4 bity
fb0b 77        ld      (hl),a		;uloû zpÏt
fb0c 2125ff    ld      hl,0ff25h	;ûe by buffer kl·vesnice?
fb0f 85        add     a,l		;p¯iËti hodnotu
fb10 6f        ld      l,a		;upraven· adresa v HL
fb11 dbd4      in      a,(0d4h)		;FDC-PIO-kan·l A-Data 
fb13 2f        cpl			;p¯evraù bity     
fb14 77        ld      (hl),a		;uloû
fb15 e1        pop     hl		;obnov registry
fb16 f1        pop     af
fb17 fb        ei			;povol dalöÌ p¯eruöenÌ
fb18 ed4d      reti			;a n·vrat z p¯eruöenÌ

;CONOUT - v˝stup znaku na obrazovku
;----------------------------------
fb1a dd210c00  ld      ix,000ch		;CONOUT - vypiö znak z C
fb1e 1816      jr      0fb36h		;volej adresu IX v eprom CPU desky

;LIST   - v˝stup znaku na tisk·rnu
;---------------------------------
fb20 dd215400  ld      ix,0054h		;PO1 - vyöli C na par. port kan·l 1
fb24 1810      jr      0fb36h		;volej adresu IX v eprom CPU desky

;LISTST - stav tisk·rny
;----------------------
fb26 dd215100  ld      ix,0051h		;PS1 - status par. portu kan·l 1 - A=0(busy)/255(ready) 
fb2a 180a      jr      0fb36h		;volej adresu IX v eprom CPU desky

;PUNCH  - v˝stup znaku na dÏrovaË
;--------------------------------
fb2c dd214e00  ld      ix,004eh		;SO1B - vyöli data z A do CPU SIO-B-Data
fb30 1804      jr      0fb36h		;volej adresu IX v eprom CPU desky

;READER - vstup znaku z dÏrnÈ p·sky
;----------------------------------
fb32 dd214b00  ld      ix,004bh		;SI1B - Ëti data do A ze CPU SIO-B-Data

;spoleËnÏ CONOUT,LIST,LISTST,PUNCH,READER
;----------------------------------------
;volej adresu IX v eprom CPU desky
;---------------------------------
fb36 f3        di			;z·kaz p¯eruöenÌ
fb37 3e03      ld      a,03h
fb39 d3d6      out     (0d6h),a		;FDC PIO kan·l A ¯ÌzenÌ 
fb3b ed733eff  ld      (0ff3eh),sp	;uloû SP
fb3f 310000    ld      sp,0000h
fb42 af        xor     a		;0 0 00 00 00
fb43 d3fc      out     (0fch),a		;CPU str·nkovacÌ registr
fb45 cd58fb    call    0fb58h		;call IX
fb48 08        ex      af,af'		;p¯epni A
fb49 3e80      ld      a,80h		;1 0 00 00 00
fb4b d3fc      out     (0fch),a		;CPU str·nkovacÌ registr
fb4d ed7b3eff  ld      sp,(0ff3eh)	;obnov SP
fb51 3e83      ld      a,83h
fb53 d3d6      out     (0d6h),a		;FDC PIO kan·l A ¯ÌzenÌ 
fb55 08        ex      af,af'		;p¯epni zpÏt A
fb56 fb        ei			;povol p¯eruöenÌ
fb57 c9        ret			;a n·vrat

;call IX
;-------
fb58 dde9      jp      (ix)		;skoË na adresu z IX

;SELDSK - v˝bÏr jednotky
;-----------------------
fb5a 210000    ld      hl,0000h		;p¯Ìznak jednotka neexistuje
fb5d 3a39fa    ld      a,(0fa39h)	;vyzvedni poËet obsluhovan˝ch jednotek (4)
fb60 b9        cp      c		;porovnej s poûadovanou jednotkou
fb61 d8        ret     c		;n·vrat pokud nenÌ 0-4
fb62 69        ld      l,c		;ËÌslo do HL
fb63 29        add     hl,hl		;2x
fb64 29        add     hl,hl		;4x
fb65 29        add     hl,hl		;8x
fb66 29        add     hl,hl		;16x
fb67 1188fc    ld      de,0fc88h	;b·ze - tabulka DHT
fb6a 19        add     hl,de		;p¯iËti
fb6b eb        ex      de,hl		;v˝sledek do DE
fb6c 79        ld      a,c		;jednotka do A
fb6d 32d5fa    ld      (0fad5h),a	;a na promÏnnou
fb70 210a00    ld      hl,000ah		;10 - pozice DPB
fb73 19        add     hl,de		;p¯iËti k tabulce disku
fb74 4e        ld      c,(hl)		;vyzvedni adresu DPB do BC
fb75 23        inc     hl
fb76 46        ld      b,(hl)
fb77 0b        dec     bc		;sniû
fb78 ed433cff  ld      (0ff3ch),bc	;uloû sem
fb7c eb        ex      de,hl		;do HL adresa popisovaËe disku
fb7d c9        ret			;a n·vrat

;HOME   - nastavenÌ ËÌsla stopy na 0
;-----------------------------------
fb7e 216cfc    ld      hl,0fc6ch	; pro ¯adiË
fb81 cbe6      set     4,(hl)
fb83 010000    ld      bc,0000h		;stopa 0 a pokraËuj v nastavenÌ stopy

;SETTRK - nastavenÌ stopy
;------------------------
fb86 ed4336ff  ld      (0ff36h),bc	;uloû na promÏnnou
fb8a c9        ret			;a n·vrat

;SETSEC - nastavenÌ sektoru
;--------------------------
fb8b 79        ld      a,c		;ËÌslo do A
fb8c 3235ff    ld      (0ff35h),a	;a na promÏnnou
fb8f c9        ret			;n·vrat

;SECTRAN - p¯eklad adresy sektoru
;--------------------------------
fb90 69        ld      l,c
fb91 60        ld      h,b
fb92 7a        ld      a,d
fb93 b3        or      e
fb94 23        inc     hl
fb95 c8        ret     z
fb96 2b        dec     hl
fb97 19        add     hl,de
fb98 6e        ld      l,(hl)
fb99 60        ld      h,b
fb9a c9        ret     

;SETDMA
;------
fb9b ed433aff  ld      (0ff3ah),bc	;uloû na promÏnnou
fb9f c9        ret			;a n·vrat

;WRITE - z·pis sektoru
;---------------------
fba0 3e02      ld      a,02h		;do A dej 2
fba2 b1        or      c		;p¯idej C
fba3 06        db      6		;p¯eskoËenÌ n·sl. instrukce  pomocÌ ld b,0afh

;READ - ËtenÌ sektoru
;--------------------
fba4 af        xor     a
fba5 216cfc    ld      hl,0fc6ch	;adresa promÏnnÈ
fba8 47        ld      b,a		;p¯Ìznak operace do B
fba9 7e        ld      a,(hl)
fbaa e61c      and     1ch		;maska 0001 1100
fbac b0        or      b		;p¯idej operaci 
fbad 77        ld      (hl),a		;uloû zpÏt
fbae 3a6dfc    ld      a,(0fc6dh)	;poslednÌ pracovnÌ jednotka
fbb1 47        ld      b,a		;do B
fbb2 3ad5fa    ld      a,(0fad5h)	;aktu·lnÌ jednotka
fbb5 326dfc    ld      (0fc6dh),a	;nastav jako poslednÌ pracovnÌ
fbb8 b8        cp      b		;porovnej
fbb9 c415fc    call    nz,0fc15h	;pokud se nerovnajÌ,zruö diskovÈ p¯Ìznaky (na HL) 3,4 a pokud je 2 zapiö buffer
fbbc d9        exx     
fbbd 2a3cff    ld      hl,(0ff3ch)	;adresa dodateËn˝ch parametr˘ aktu·lnÌ jednoty
fbc0 7e        ld      a,(hl)		;vyzvedni poslednÌ
fbc1 fe20      cp      20h		;je to RAMdisk?
fbc3 2878      jr      z,0fc3dh		;odskoË pokud ano
fbc5 dd211e00  ld      ix,001eh		;SDRIVE - vyber diskovou jednotu
fbc9 cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fbcc 2136ff    ld      hl,0ff36h	;promÏnn· ËÌslo stopy
fbcf 7e        ld      a,(hl)		;vyzvedni do A
fbd0 23        inc     hl		;ff37
fbd1 23        inc     hl		;ff38
fbd2 be        cp      (hl)		;porovnej
fbd3 77        ld      (hl),a		;a nastav novou hodnotu sem
fbd4 d9        exx     
fbd5 2802      jr      z,0fbd9h		;poskoË pokud se rovnaly
fbd7 cbe6      set     4,(hl)		;jinak nastav - v HL je FC6C
fbd9 cb66      bit     4,(hl)
fbdb c415fc    call    nz,0fc15h	;pokud je nastaven zruö diskovÈ p¯Ìznaky (na HL) 3,4 a pokud je 2 zapiö buffer
fbde d9        exx     
fbdf 2a38ff    ld      hl,(0ff38h)
fbe2 dd211500  ld      ix,0015h		;SEEKTR - Hlava na poûadovanou stopu
fbe6 cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fbe9 d9        exx     
fbea cb5e      bit     3,(hl)
fbec cbde      set     3,(hl)
fbee dd211800  ld      ix,0018h		;FREAD - p¯eËti jednu stopu (na HL)
fbf2 cc22fc    call    z,0fc22h
fbf5 d9        exx     
fbf6 2a3afa    ld      hl,(0fa3ah)	;vyzvedni adresu bufferu (volno ve 3. str·nce od ED00h)
fbf9 3a35ff    ld      a,(0ff35h)	;vyzvedni ËÌslo sektoru
fbfc 3d        dec     a		;poËÌt·me od 0
fbfd cb3f      srl     a		;
fbff 57        ld      d,a
fc00 3e00      ld      a,00h
fc02 1f        rra     
fc03 5f        ld      e,a
fc04 19        add     hl,de
fc05 eb        ex      de,hl		;v˝sledek do DE
fc06 3e03      ld      a,03h		;0 0 00 00 11
fc08 cd49fc    call    0fc49h
fc0b cb4e      bit     1,(hl)
fc0d c8        ret     z
fc0e cbd6      set     2,(hl)
fc10 cb46      bit     0,(hl)
fc12 c8        ret     z
fc13 1802      jr      0fc17h		;a poskoË

;zruö diskovÈ p¯Ìznaky (na HL) 3,4 a pokud je 2 zapiö buffer
;-----------------------------------------------------------
fc15 cb9e      res     3,(hl)		;zruö bit3
fc17 cba6      res     4,(hl)		;zruö bit4
fc19 cb56      bit     2,(hl)		;bit2
fc1b c8        ret     z		;n·vrat pro 0
fc1c cb96      res     2,(hl)
fc1e dd211b00  ld      ix,001bh		;FWRITE - zapiö jednu stopu (z HL)
;spoleËnÏ FREAD,FWRITE tudy 
fc22 0603      ld      b,03h		;poËÌtadlo pokus˘
fc24 d9        exx     
fc25 dde5      push    ix		;uschovat adresu rutiny v Eprom
fc27 1601      ld      d,01h
fc29 3e3f      ld      a,3fh		;0 0 11 11 11
fc2b 08        ex      af,af'
fc2c 2a3afa    ld      hl,(0fa3ah)	;vyzvedni adresu bufferu (volno ve 3. str·nce od ED00h)
fc2f cd36fb    call    0fb36h		;volej adresu IX v eprom CPU desky
fc32 dde1      pop     ix		;obnov adresu rutiny v Eprom
fc34 d9        exx     
fc35 a7        and     a		;v˝sledek
fc36 c8        ret     z		;n·vrat pokud je vöe OK
fc37 10eb      djnz    0fc24h		;opakuj 3x
fc39 3e01      ld      a,01h		;1 - p¯Ìznak chyby
fc3b e1        pop     hl		;zahoÔ norm·lnÌ n·vrat
fc3c c9        ret			;a vraù se o ˙roveÚ v˝ö

;ËtenÌ/z·pis sektoru z/do RAMDISKU
;---------------------------------
fc3d 3a35ff    ld      a,(0ff35h)	;vyzvedni ËÌslo sektoru
fc40 3d        dec     a		;sniû (poËÌt·me od nuly)
fc41 0f        rrca			;dÏl 2
fc42 5f        ld      e,a		;v˝sledek do E
fc43 2a36ff    ld      hl,(0ff36h)	;vyzvedni ËÌslo stopy
fc46 55        ld      d,l		;stopa do D (niûöÌ bajt)
fc47 7c        ld      a,h		;vyööÌ bajt bude str·nkovat
fc48 3c        inc     a		;zvyö o jedna (zaËÌn·me str·nkou 1)
fc49 2a3aff    ld      hl,(0ff3ah)	;DMA
fc4c eb        ex      de,hl		;DE=DMA, HL=odkud
fc4d 018000    ld      bc,0080h		;p¯en·öÌme 128 bajt˘ (1 sektor CP/M)
fc50 d9        exx                      ;p¯ehoÔ registry
fc51 01fc80    ld      bc,80fch		;C=CPU str·nkovacÌ port, B=1 0 00 00 00
fc54 cb4e      bit     1,(hl)		;p¯Ìznak pro operace (zjisti smÏr p¯enosu)
fc56 d9        exx			;registry zpÏt			     
fc57 2803      jr      z,0fc5ch		;pro smÏr do bufferu poskoË
fc59 eb        ex      de,hl		;pro smÏr z bufferu p¯ehoÔ adresy
fc5a 07        rlca			;a posuÚ str·nkovacÌ bity    
fc5b 07        rlca    
fc5c f680      or      80h		;p¯idej bit7 (str·nkov·nÌ RAM)
fc5e f3        di			;zakaû p¯eruöenÌ      
fc5f d3fc      out     (0fch),a		;str·nkuj
fc61 edb0      ldir			;p¯enes    
fc63 d9        exx			;zpÏt registry     
fc64 ed41      out     (c),b		;nastr·nkuj str·nku 0 pro vöechny operace
fc66 af        xor     a		;p¯Ìznak vöe OK
fc67 fb        ei			;povol p¯eruöenÌ      
fc68 c9        ret			;a n·vrat

fc69 00        nop     
fc6a 00        db      0		; pro buffer kl·vesnice
fc6b 00        db      0		;pozice v bufferu kl·vesnice (0-15), zaËÌn· na FF25
fc6c 00        db      0		; pro ¯adiË
fc6d 00        db      0		;aktu·lnÌ jednotka (kopie z FAD5h)

;XLT 8"
;(p¯evod fyzick˝ch sektor˘ na logickÈ)
;-------------------------------------
fc6e 01070d13  db      1,7,13,19,25,5,11,17,23,3,9,15,21
fc72 19050b11
fc76 1703090f    
fc7a 15
fc7b 02080e14  db      2,8,14,20,26,6,12,18,24,4,10,16,22
fc7f 1a060c12
fc83 18040a10
fc87 16

;DHT disk0 - A
;-------------
fc88 0000      dw      0		;XLT
fc8a 00000000  dw      0,0,0
fc8e 0000     
fc90 3cfd      dw      0fd3ch		;DIR BUFF
fc92 ddfc      dw      0fcddh		;DPB0
fc64 bcfd      dw      0fdbch		;CSV0
fc96 bcfd      dw      0fdbch		;ALV0

;DHT disk1 - B
;-------------
fc98 0000      dw      0		;XLT
fc9a 00000000  dw      0,0,0     
fc9e 0000     
fca0 3cfd      dw      0fd3ch		;DIR BUFF
fca2 f1fc      dw      0fcf1		;DPB1
fca4 d3fd      dw      0fdd3h		;CSV1
fca6 f3fd      dw      0fdf3h		;ALV1

;DHT disk2 - C
;-------------
fca8 0000     
fcaa 00000000  dw      0,0,0
fcae 0000     
fcb0 3cfd      dw      0fd3ch		;DIR BUFF
fcb2 05fd      dw      0fd05h		;DPB2
fcb4 20fe
fcb6 40fe

;DHT disk3 - D
;-------------
fcb8 6efc      dw      0FC6Eh		;XLT
fcba 00000000  dw      0,0,0     
fcbe 0000     
fcc0 3cfd      dw      0fd3ch		;DIR BUFF
fcc2 19fd      dw      0fd19h		;DPB3
fcc4 6dfe
fcc6 7dfe

;DHT disk4 - E
;-------------
fcc8 6efc      dw      0FC6Eh		;XLT
fcca 00000000  dw      0,0,0    
fcce 0000     
fcd0 3cfd      dw      0fd3ch		;DIR BUFF
fcd2 2dfd      dw      0fd2d		;DPB4
fcd4 9cfe
fcd6 acfe

;dodateËnÈ parametry pro DPB0
;----------------------------
fcd8 00        db      0
fcd9 00        db      0
fcda 02        db      2		;max. na stopÏ
fcdb 04        db      4
fcdc 20        db      20h		;

;DPB0 - A - ramdisk 181kB
;------------------------
fcdd 0200      dw      2		;SEC PER TRACK
fcdf 03        db      3		;BLOCK SHIFT FACTOR
fce0 07        db      7		;BLOCK MASK
fce1 00        db      0		;EXTNT MASK
fce2 b400      dw      180		;DISK SIZE-1
fce4 1f00      dw      31		;DIRECTORY MAX
fce6 80        db      128		;ALLOC0
fce7 00        db      0		;ALLOC1
fce8 0000      dw      0		;CHECK SIZE
fcea 0000      dw      0		;TRACK OFFSET

;dodateËnÈ parametry pro DPB1
;----------------------------
fcec 64        db      100
fced 32        db      50
fcee 09        db      9		;max. na stopÏ
fcef 90        db      144
fcf0 8c        db      140

;DPB1 - B - 5,25"
;----------------
fcf1 2400      dw      36		;SEC PER TRACK
fcf3 04        db      4		;BLOCK SHIFT FACTOR
fcf4 0f        db      15		;BLOCK MASK    
fcf5 00        db      0		;EXTNT MASK
fcf6 6201      dw      354		;DISK SIZE-1
fcf8 7f00      dw      127		;DIRECTORY MAX
fcfa c0        db      192		;ALLOC0
fcfb 00        db      0		;ALLOC1
fcfc 2000      dw      32		;CHECK SIZE
fcfe 0200      dw      2		;TRACK OFFSET
     
;dodateËnÈ parametry pro DPB2
;----------------------------
fd00 64        db      100
fd01 32        db      50
fd02 09        db      9		;max. na stopÏ
fd03 91        db      145
fd04 8c        db      140

;DPB2 - C - 5,25"
;----------------
fd05 2400      dw      36		;SEC PER TRACK
fd07 04        db      4		;BLOCK SHIFT FACTOR
fd08 0f        db      15		;BLOCK MASK    
fd09 00        db      0		;EXTNT MASK
fd0a 6201      dw      354		;DISK SIZE-1
fd0c 7f00      dw      127		;DIRECTORY MAX
fd0e c0        db      192		;ALLOC0
fd0f 00        db      0		;ALLOC1
fd10 2000      dw      32		;CHECK SIZE
fd12 0200      dw      2		;TRACK OFFSET

;dodateËnÈ parametry pro DPB3
;----------------------------
fd14 00        db      0
fd15 00        db      0
fd16 1a        db      26		;max. na stopÏ
fd17 0e        db      14
fd18 07        db      7

;DPB3 - D - 8"
;-------------
fd19 1a00      dw      26		;SEC PER TRACK
fd1b 03        db      3		;BLOCK SHIFT FACTOR
fd1c 07        db      7		;BLOCK MASK
fd1d 00        db      0		;EXTNT MASK
fd1e f200      dw      242		;DISK SIZE-1
fd20 3f00      dw      63		;DIRECTORY MAX
fd22 c0        db      192		;ALLOC0
fd23 00        db      0		;ALLOC1
fd24 1000      dw      16		;CHECK SIZE
fd26 0200      dw      2		;TRACK OFFSET

;dodateËnÈ parametry pro DPB4
;----------------------------
fd28 00        db      0
fd29 00        db      0
fd2a 1a        db      26		;max. na stopÏ
fd2b 0f        db      15
fd2c 07        db      7

;DPB4 - E - 8"
;-------------
fd2d 1a00      dw      26		;SEC PER TRACK
fd2f 03        db      3		;BLOCK SHIFT FACTOR
fd30 07        db      7		;BLOCK MASK
fd31 00        db      0		;EXTNT MASK
fd32 f200      dw      242		;DISK SIZE-1
fd34 3f00      dw      63		;DIRECTORY MAX
fd36 c0        db      192		;ALLOC0
fd37 00        db      0		;ALLOC1
fd38 1000      dw      16		;CHECK SIZE
fd3a 0200      dw      2		;TRACK OFFSET


;Coldboot - pozdÏji DIRBUFF
;--------------------------
fd3c f3        di			;z·kaz p¯eruöenÌ      
fd3d 310000    ld      sp,0000h		;z·sobnÌk na konec RAM
fd40 21fafa    ld      hl,0fafah	;adresa rutiny p¯eruöenÌ IM2 - ËtenÌ dat z FDC-PIO-A do bufferu
fd43 228aff    ld      (0ff8ah),hl	;nastav pro vektor 8A
fd46 217dfd    ld      hl,0fd7dh	;inicializaËnÌ data FDC-PIO-A-¯ÌzenÌ
fd49 cd4500    call    0045h		;vysÌl·nÌ dat z HL na port (dÈlka/0,port,data)
fd4c 2183fd    ld      hl,0fd83h	;˙vodnÌ text
fd4f cd0f00    call    000fh		;vypiö
fd52 114dfa    ld      de,0fa4dh	;adresa p¯Ìznaku (je tam 32 po startu)
fd55 1a        ld      a,(de)		;vyzvedni
fd56 fe20      cp      20h		;je to 32?
fd58 2011      jr      nz,0fd6bh	;poskoË pokud ne
fd5a 21fff9    ld      hl,0f9ffh	;odkud
fd5d ed5b3afa  ld      de,(0fa3ah)	;kam (ve 3. str·nce)
fd61 1b        dec     de		;o jedno nÌû
fd62 010016    ld      bc,1600h		;kolik
fd65 3e8c      ld      a,8ch		;1 0 00 11 00 (zapni str·nkov·nÌ, 0 ËtenÌ,bÏh + 3 z·pis)
fd67 d3fc      out     (0fch),a		;deska CPU str·nkovacÌ registr
fd69 1809      jr      0fd74h		;a poskoË na p¯enos
fd6b a7        and     a		;je v A 0?
fd6c 2008      jr      nz,0fd76h	;poskoË na dokonËenÌ pokud ne
fd6e 211222    ld      hl,2212h		;odkud (CPU RAM)
fd71 010500    ld      bc,0005h		;kolik
fd74 edb8      lddr			;p¯enes
fd76 3e80      ld      a,80h		;1 0 00 00 00 - odepni CPU Ram/Rom a vöe ve str·nce 0
fd78 d3fc      out     (0fch),a		;deska CPU str·nkovacÌ registr	
fd7a c3b8fa    jp      0fab8h		;a pokraËuj tudy - inicializuj skoky a start CCP

;inicializaËnÌ data
;------------------
fd7d 03d6      db      3,0D6h		;dÈlka 3, port D6 = FDC-PIO-kan·l A-¯ÌzenÌ 
fd7f 8a        db      8ah		;hodnota vektoru IM2 = 8Ah
fd80 4f        db      4fh
fd81 83        db      83h
fd82 00        db      0		;koncov· znaËka

;˙vodnÌ text
;-----------
fd83 1f        db      1Fh		;CRLF
fd84 36346b20  db      "64k CP/M Z256 Bios 3.09 Date: 17.10.89"
fd88 43502f4d
fd8c 205a3235
fd90 36204269
fd94 6f732033
fd98 2e303920
fd9c 44617465
fda0 3a203137     
fda4 2e31302e
fda8 3839
fdaa 1f1f      db      1Fh,1Fh		;2xCRLF
fdac 41202d20  db      "A - 181k RAM  - BOOT"
fdb0 3138316b
fdb4 2052414d
fdb8 20202d20
fdbc 424f4f54
fdc0 1f        db      1Fh		;CRLF
fdc1 42202d20  db      "B - 5",34," DS/DD "
fdc5 35222044
fdc9 532f4444
fdcd 20
fdce 1f        db      1Fh		;CRLF
fdcf 43202d20  db      "C - 5",34," DS/DD "
fdd3 35222044
fdd7 532f4444
fddb 20
fddc 1f        db      1Fh		;CRLF
fddd 44202d20  db      "D - 8",34,"" SS/SD "
fde1 38222053
fde5 532f5344
fde9 20
fdea 1f        db      1Fh		;CRLF
fdeb 45202d20  db      "E - 8",34,"" SS/SD "
fdef 38222053
fdf3 532f5344
fdf7 20
fdf8 00        db      0		;konec textu

fdf9 28632920  db      "(c) 1988 by Petr Kotek"
fdfd 31393838
fe01 20627920
fe05 50657472
fe09 204b6f74
fe0d 656b

;volnÈ mÌsto
;-----------
fe0f-feff 00  ds     xx x 00

;mÌsto pro rutiny p¯eruöenÌ a jejich vektory
;------------------------------------------- 
ff00-ffff 00  ds     256x00

;buffer kl·vesnice
;-----------------
ff25-ff34 00  ds     16x00         
