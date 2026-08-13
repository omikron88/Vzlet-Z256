# Technická specifikace emulovaného počítače

Stav dokumentu: pracovní specifikace sestavená z popisu skutečného zapojení.

## 1. Základní vlastnosti

- CPU: Z80, 2 nebo 4 MHz.
- Periferní obvody pocházejí převážně z rodiny Z80:
  - Z80 PIO,
  - Z80 SIO,
  - Z80 CTC,
  - Floppy řadič WD2797.
  - Video řadič MC6845.
  - BIOS počítače má 2-8KB ROM + 8KB SRAM.
- Deska RAM 256KB se stránkováním.
- Video: černobílé, 640×300 pixelů, 2 bity na pixel,úrovně šedi, PAL 50 Hz, obraz 4:3.
  Video deska má vlastní RAM 128KB.
- Disky: Dvě 5.25palcové mechaniky, selekt 0,1: 80 stop, double side,
         dvě 8palcové mechaniky, selekt 2,3: 77 stop, double side.

- Jsou k dispozici schémata ve formě obrázků, obsahy EPROM, zdrojové kódy EPROM a CP/M BIOSu. V adresáři /doc

## 2. Deska CPU

Deska CPU obsahuje kromě procesoru Z80 také přípravu na vestavění obvodu Z80 DMA (adresa 0xEC). V existujícím software není využit, pouze v inicializační rutine se pošle instrukce pro reset. Proto není nutné jej prozatím emulovat.

V celém adresním rozsahu 64KB procesoru je paměť RAM. To, která stránka z 256KB RAM bude připojená, určuje port na adrese 0xFC:
  - bity 1,0: číslo stránky pro čtení z paměti
  - bity 3,2: číslo stránky pro zápis do paměti
  - bity 5,4: číslo stránky pro čtení instrukce (memrq+m1)
  - bit 6: nepoužit
  - bit 7: Monitor. 0= v rozsahu prvních 16KB, 0x0000 - 0x3FFF je připojen Monitor.
Celý registr se nuluje nejen signálem Reset, ale i příchodem NMI. Tím je zajištěno připojení Monitoru po spuštění, ale i pro nemaskovatelné přerušení.

Monitorem se rozumí připojení:
  - EPROM od 0x0000 - 0x1FFF, pokud je použita menší než 8KB, tak se v rozsahu zrcadlí
  - SRAM od 0x2000 - 0x3FFF.
  - zbytek paměťového rozsahu je beze změny, stále dle bitů 5-0.

Sekundární stránkovací port na adrese 0xC0-0xCF přepíná mezi hlavní RAM a videořadičem. Nezachytávají se do něj data, nýbrž stav adres A0-A3:
  - A0: 0=Je připojena hlavní RAM. 1=Místo hlavní RAM jsou připojeny obvody videořadiče.
  - A1: 1=Zapíná kontrolu parity hlavní RAM. Chyba parity vyvolá NMI. 0=Parita se nekontroluje.
  - A2: rezervováno pro budoucí rozšíření paměti.
  - A3: rezervováno pro budoucí rozšíření paměti.
Registr je nulován signálem Reset.
Hlavní registr stránkování 0xFC má přednost. I když jsou přepnuty obvody zobrazení, stále funguje logika oddělené stránky pro čtení, zápis a čtení instrukce.
Jen to není v hlavní RAM, ale v obvodech zobrazení. Stejně tak bit Monitor funguje stejně.

### CPU CTC

Časovač na desce CPU na adresách 0xF4-0xF7.
  - Kanál 0: Na vstupu 4MHz vydělené 13. Výstup vede na vstup přenosové rychlosti CPU SIO kanál A.
  - Kanál 1: Na vstupu 4MHz vydělené 13. Výstup vede na vstup přenosové rychlosti CPU SIO kanál B.
  - Kanál 2: Vstup nezapojen, výstup nezapojen. Používá se v režimu časovač.
  - Kanál 3: Vstup nezapojen, výstup nezapojen. Používá se v režimu časovač.

### CPU SIO

Dvojitý sériový port na desce CPU na adresách 0xF8-0xFB. B/-A = A0, C/-D = A1.
Je celý vyveden na dva běžné sériové porty a může být použit například pro spojení s jiným emulátorem přes TCP loopback.

### CPU PIO

Paralelní port na desce CPU na adresách 0xF0-0xF3. B/-A = A0, C/-D = A1. Používá se pro připojení Centronix tiskárny:
  -DATA 0 to 7 - channel B bit 0 to 7 (output active High)
  -BUSY        - channel A bit 0      (input  active High)
  -PE          - channel A bit 1      (input  active High)
  -ERROR       - channel A bit 2      (input  active Low )
  -STROBE      - channel A bit 7      (output active Low )

Prioritní řetězec přerušení na desce CPU: ->-SIO->-CTC->-DMA->-PIO->- Pokud není DMA osazen, je přemostěn spojkou.

## 3. Deska CRTC

Na desce je řadič zobrazení MC6845, EPROM 2-8KB s tvary znaků, a dvě banky paměti po 64KB. 
Nejsou zde žádné I/O porty, vše je mapováno jako paměť.
Video pracuje stále v grafickém režimu, není žádný textový mód.
Pixely se vysílají kmitočtem 12.5MHz. Efektivní rozlišení je 640*300, bitová hloubka 2 bity, 4 úrovně šedi.
Každý bit určující jas pixelu je v jiné paměťové bance na stejné pozici. Je to tedy planární videopaměť

  - Banka 0: obsahuje EPROM s tvary znaků, zrcadlí se stále dokola v celém rozsahu
  - Banka 1: obsahuje vyšší bity jasu pixelu. Po osmicích MSB first.
  - Banka 2: obsahuje nižší bity jasu pixelu. Po osmicích MSB first.
  - Banka 3: obsahuje MC6845 mapovaný jako paměť. Vstup RS je připojený na A0. Zrcadlí se v celém rozsahu.
  
Banky 1 a 2 jsou video obvody vyčítány jen v horní polovině, na adresách 0x8000-0xFFFF. Z pohledu procesoru jsou ale přístupné celé.
  
## 3. Deska FDC

Deska obsahuje řadič 4 floppy mechanik s obvodem WD2797, další CTC, další SIO, a PIO pro připojení paralelní Ascii klávesnice.
Také obsahuje obvody pro záznam dat na kazetový magnetofon, včetně relé pro zapínání motoru.

### FDC CTC

Časovač na desce FDC na adresách 0xD8-0xDB.
  - Kanál 0: Na vstupu 1MHz. Výstup vede na CLK vstup klopného obvodu QQQ zapojeného jako dělička dvěma.
		Z výstupu QQQ vede na vstup přenosové rychlosti FDC SIO kanál A i B.
  - Kanál 1: Na vstup vede signál z komparátoru levého kanálu z pásky. Výstup při úrovni 1 resetuje klopný obvod QQQ.
  - Kanál 2: Na vstup vede také signál z komparátoru levého kanálu z pásky. Výstup při úrovni 1 nastavuje klopný obvod QQQ.
  - Kanál 3: Vstup nezapojen, výstup nezapojen. Používá se v režimu časovač.


### FDC SIO

Dvojitý sériový port na desce FDC na adresách 0xDC-0xDF. B/-A = A1, C/-D = A0.
	- Kanál A
		- Výstup TxD A je vyveden ven pro použití jako sériový port. Zároveň vede přes XOR hradlo na výstup levého kanálu pásky.
			Na druhý vstup XOR hradla vede výstup klopného obvodu QQQ.
		- Výstup RTS A se používá pro relé posunu pásky, neaktivní stav = páska jede.
		- Výstup DTR A je vyveden ven pro řízení přenosu sériového portu Zároveň vede do vstupních obvodů levého kanálu pásky.
		- Vstup RxD A se přepíná multiplexorem buď jako vstup ze sériového portu, nebo sem vede přes XOR hradlo signál z komparátoru levého kanálu z pásky.
			Na druhý vstup XOR hradla vede signál DTR A. 
		- Vstup CTS A se přepíná multiplexorem buď jako vstup řízení přenosu sériového portu, nebo sem vede signál z komparátoru levého kanálu z pásky.
		- Vstup DCD A trvale v aktivní úrovni.
		
	- Kanál B
		- Výstup TxD B je vyveden ven pro použití jako sériový port. Zároveň vede přes XOR hradlo na výstup pravého kanálu pásky.
			Na druhý vstup XOR hradla vede výstup klopného obvodu QQQ.
		- Výstup RTS B přepíná multiplexor vstupních signálů obou kanálů. Aktivní stav = sériový port, neaktivní stav = vstupy z pásky.  
		- Výstup DTR B je vyveden ven pro řízení přenosu sériového portu Zároveň vede do vstupních obvodů pravého kanálu pásky.
		- Vstup RxD B se přepíná multiplexorem buď jako vstup ze sériového portu, nebo sem vede přes XOR hradlo signál z komparátoru pravého kanálu z pásky.
			Na druhý vstup XOR hradla vede signál DTR B. 
		- Vstup CTS B se přepíná multiplexorem buď jako vstup řízení přenosu sériového portu, nebo sem vede signál z komparátoru pravého kanálu z pásky.
		- Vstup DCD B trvale v aktivní úrovni.

Záznam na magnetofon tedy používá fázovou modulaci, kde stav 0 je kódován jako "10" a stav 1 jako "01".

### FDC PIO

Paralelní port na desce FDC na adresách 0xD4-0xD7. B/-A = A0, C/-D = A1.

Celý port A se používá jako vstupní port pro připojení paralelní Ascii klávesnice s handshakingem pomocí ASTB a ARDY.
Port B se používá jako diskrétní signály pro řízení různých funkcí:
  -1,0: výstup, binárně vybírají floppy mechaniku 0-3.
  -2: výstup, ovládá vstup /DDEN WD2797.
  -3: výstup, ovládá vstup 8/5 WD2797.
  -4: výstup, ovládá signál Motor on, aktivní v 1.
  -5: výstup, ovládá signál In use, aktivní v 1.
  -6: vstup, připojen na výstup INT WD2797, aktivní v 1.
  -7: vstup, připojen na výstup DRQ WD2797, aktivní v 1.
  
 Výstup INT z WD2797, naní připojen na přerušení procesoru, ale do FDC PIO. Tak může vyvolat přerušení podle pravdiel IM2.
 
 ### FDC WD2797

Floppy řadič WD2797 na adresách 0xD0-0xD3. Je zapojen víceméně katalogovým způsobem.
Pouze signál vstupní signál HLT (hlava přiklopena) je generován pomocí monostabilního klopného obvodu z výstupního signálu HLD (přiklop hlavu).
Protože novější mechaniky tento signál nemívají. Časová konstanta je asi 0.4s.

Prioritní řetězec přerušení na desce FDC: ->-PIO->-CTC->-SIO->-



