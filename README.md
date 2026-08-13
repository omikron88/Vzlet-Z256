# Emulátor Vzlet Z-256

Přenosný základ emulátoru historického počítače Vzlet Z-256. Projekt používá C++20,
SDL 3 pro okno, vstup a výstup obrazu a procesorové jádro
[redcode/Z80](https://github.com/redcode/Z80). Zapojení vychází z
[`hardware-spec.md`](hardware-spec.md) a dobových podkladů v adresáři [`doc`](doc).

## Co je implementováno

- 256 KiB operační RAM a nezávislé stránky pro čtení, zápis a M1 fetch (`0xFC`),
- monitorová EPROM/SRAM v prvních 16 KiB včetně zrcadlení kratší EPROM,
- sekundární stránkování `0xC0–0xCF`, 128 KiB planární VRAM a paměťové registry MC6845,
- převod dvou bitových rovin na 640×300 ve čtyřech odstínech šedi a zobrazení 4:3,
- obraz disket s bezpečným adresováním 512b sektorů a základ paralelní ASCII klávesnice,
- adaptér procesoru redcode/Z80, real-time smyčka na 4 MHz, reset klávesou **F12**
  a automatické načtení dodaných ROM/disku.

PIO, SIO, CTC a WD2797 mají připravené dekódování sběrnice, ale jejich stavové automaty,
přerušovací daisy-chain a přesné časování jsou další etapou. Tento stav je záměrně
oddělen od paměťového a obrazového jádra, které lze testovat bez SDL.

## Sestavení

Je třeba CMake 3.24+, překladač s C++20 a vývojová verze SDL 3. CMake stáhne
`redcode/Z80`; v prostředí bez sítě lze předat již existující checkout:

```sh
cmake -S . -B build -DVZ256_Z80_SOURCE_DIR=/cesta/k/Z80
cmake --build build
./build/vz256 --resources .
```

Samotné jádro a testy nemají externí závislosti:

```sh
cmake -S . -B build -DVZ256_BUILD_APP=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Návrh dalších etap

1. WD2797: příkazy Type I–IV, DRQ/INTRQ, geometrie 80/77 stop a přepínání hustoty.
2. Z80 PIO/CTC/SIO: režimy, vektory IM2 a dva prioritní řetězce podle specifikace.
3. MC6845: odvozovat počáteční adresu a časování snímku z registrů namísto pevného rastru.
4. TCP sériové linky, tisk do souboru, magnetofonní WAV a debugger CPU/paměti.

ROM a diskové obrazy se před ukončením nemění, kromě připojeného zapisovatelného obrazu
mechaniky 0, který emulátor uloží pouze tehdy, když byl úspěšně připojen.
