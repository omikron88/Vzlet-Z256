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
  bity 1,0: číslo stránky pro čtení z paměti
  bity 3,2: číslo stránky pro zápis do paměti
  bity 5,4: číslo stránky pro čtení instrukce (memrq+m1)
  bit 6: nepoužit
  bit 7: Monitor. 0= v rozsahu prvních 16KB, 0x0000 - 0x3FFF je připojen Monitor.
Celý registr se nuluje nejen signálem Reset, ale i příchodem NMI. Tím je zajištěno připojení Monitoru po spuštění, ale i pro nemaskovatelné přerušení.
Monitorem se rozumí připojení:
  - EPROM od 0x0000 - 0x1FFF, pokud je použita menší než 8KB, tak se v rozsahu zrcadlí
  - SRAM od 0x2000 - 0x3FFF.
  - zbytek paměťového rozsahu je beze změny, stále dle bitů 5-0.

Sekundární stránkovací port na adrese 0xC0-0xCF přepíná mezi hlavní RAM a videořadičem. Nezachytávají se do něj data, nýbrž stav adres A0-A3.
  A0: 0=Je připojena hlavní RAM. 1=Místo hlavní RAM jsou připojeny obvody videořadiče.
  A1: 1=Zapíná kontrolu parity hlavní RAM. Chyba parity vyvolá NMI. 0=Parita se nekontroluje.
  A2: rezervováno pro budoucí rozšíření paměti.
  A3: rezervováno pro budoucí rozšíření paměti.
Registr je nulován signálem Reset.



