# Emulátor Vzlet Z-256

Přenosný základ emulátoru historického počítače Vzlet Z-256. Projekt používá C++20,
SDL 3 pro okno, vstup a výstup obrazu a procesorové jádro
[redcode/Z80](https://github.com/redcode/Z80). Zapojení vychází z
[`hardware-spec.md`](hardware-spec.md) a dobových podkladů v adresáři [`doc`](doc).

## Co je implementováno

- 256 KiB operační RAM a nezávislé stránky pro čtení, zápis a M1 fetch (`0xFC`),
- monitorová EPROM/SRAM v prvních 16 KiB včetně zrcadlení kratší EPROM,
- sekundární stránkování `0xC0–0xCF`, 128 KiB planární VRAM a paměťové registry MC6845,
- převod dvou bitových rovin na 640×300 ve čtyřech odstínech šedi, včetně
  adresního prokládání MA/RA a počáteční adresy obrazu řízené MC6845,
- obraz disket s bezpečným adresováním 512b sektorů,
- paralelní aktivně nízká ASCII klávesnice přes PIO A, ASTB přerušení, fronta znaků
  a SDL mapování Ctrl, kurzorových a speciálních kláves,
- WD2797 s příkazy Restore/Seek/Step, Read/Write Sector, multi-sector přenosem,
  Read Address, Force Interrupt a signály DRQ/INTRQ přes Z80 PIO B,
- adaptér procesoru redcode/Z80, real-time smyčka na 4 MHz, reset klávesou **F12**
  a automatické načtení dodaných ROM/disku.

PIO, SIO a CTC mají připravené dekódování sběrnice, ale jejich úplné stavové automaty,
přerušovací daisy-chain a přesné časování jsou další etapou. Tento stav je záměrně
oddělen od paměťového a obrazového jádra, které lze testovat bez SDL.

## Sestavení

Je třeba CMake 3.24+ a překladač s C++20. CMake stáhne připnuté revize SDL 3,
`redcode/Z80` a jeho závislosti Zeta. Systémové SDL3 použije automaticky;
v prostředí bez sítě lze předat již existující checkout Z80 a vypnout stažení SDL:

```sh
cmake -S . -B build -DVZ256_Z80_SOURCE_DIR=/cesta/k/Z80 -DVZ256_FETCH_SDL3=OFF
cmake --build build
./build/vz256 --resources .
```

Obrazy a geometrie všech mechanik lze zadat samostatně (písmena `a` až `d`):

```sh
./build/vz256 --resources . \
  --drive-a disks/boot.img --geometry-a 5.25-dsdd-80 \
  --drive-c disks/system8.img --geometry-c 8-dssd-77 --read-only-c
```

Známé profily jsou `5.25-dsdd-80`, `5.25-dsdd-40`, `8-sssd-77`, `8-dssd-77`
a `8-dsdd-77`. Bez parametru `--geometry-X` se profil jednoznačně rozpozná podle
velikosti obrazu; neznámá nebo nejednoznačná velikost je bezpečně odmítnuta.

Samotné paměťové a obrazové jádro lze testovat bez externích závislostí:

```sh
cmake -S . -B build -DVZ256_BUILD_APP=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Při běžném sestavení se navíc spustí integrační test skutečného jádra Z80. Testovací
program provede opcode/data fetch, zápis do RAM a výstup na stránkovací port, takže
ověřuje nejen linkování knihovny, ale také celý adaptér sběrnice.

Test `boot_disk` spustí dodanou monitorovou EPROM, provede inicializaci periferií a
ověří, že BIOS přes přerušení PIO/WD2797 načte z `boot.img` jak diskový loader do
`0xF000`, tak CCP/BDOS do `0xE400`, vypíše prompt CP/M a přejde do čekání na
klávesnici v rutině BIOS CONIN. Následně přes PIO klávesnici zadá `DIR` a ověří
návrat na další prompt.

## Návrh dalších etap

1. WD2797: doplnit Read/Write Track, CRC, reálné časování a geometrii 77stopých 8\" disků.
2. Z80 PIO/CTC/SIO: režimy, vektory IM2 a dva prioritní řetězce podle specifikace.
3. MC6845: odvozovat počáteční adresu a časování snímku z registrů namísto pevného rastru.
4. TCP sériové linky, tisk do souboru, magnetofonní WAV a debugger CPU/paměti.

ROM a diskové obrazy se před ukončením nemění, kromě připojeného zapisovatelného obrazu
mechaniky 0, který emulátor uloží pouze tehdy, když byl úspěšně připojen.
