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
- hardwarový kurzor MC6845 řízený registry R10/R11/R14/R15, včetně vypnutí,
  obtékajícího rozsahu rastrových řádků a režimů blikání po 16 nebo 32 snímcích,
- obrazy disket s konfigurovatelnou geometrií a sektory 128, 256 nebo 512 bajtů,
- paralelní aktivně nízká ASCII klávesnice přes PIO A, ASTB přerušení, fronta znaků
  a SDL mapování Ctrl, kurzorových a speciálních kláves,
- WD2797 s příkazy Restore/Seek/Step, Read/Write Sector, multi-sector přenosem,
  Read Address, Force Interrupt a signály DRQ/INTRQ přes Z80 PIO B,
- časový limit diskových operací přes kanál 3 CPU CTC, takže přístup k prázdné
  mechanice skončí chybou BIOSu namísto trvalého čekání,
- čtyřkanálový CPU Z80 CTC v režimu timer/counter, prescalery 16/256, čtení
  čítače, kaskáda kanálů 2→3 a vektorovaná přerušení IM2,
- adaptér procesoru redcode/Z80, real-time smyčka na 4 MHz, reset klávesou **F12**
  a automatické načtení dodaných ROM/disku.

Klávesa **F10** otevře správu čtyř mechanik. Šipky nahoru/dolů vybírají mechaniku,
šipky vlevo/vpravo geometrii pro příští obraz, **Enter** nebo **O** otevře systémový
výběr souboru, **S** obraz uloží, **E** jej vysune a **W** přepíná ochranu proti
zápisu. U změněného obrazu je před vysunutím nutné změny uložit nebo výslovně
zahodit. Zápis obrazu používá dočasný soubor a atomické přejmenování.

PIO, SIO a druhý CTC na FDC desce mají připravené dekódování sběrnice, ale jejich
úplné stavové automaty a přerušovací daisy-chain jsou další etapou. Tento stav je záměrně
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

### Ubuntu 24.04 a Raspberry Pi OS

Pro Ubuntu 24.04 na x86-64 i ARM64 a aktuální Raspberry Pi OS na Raspberry Pi 4/5
je připraven instalační skript a CMake preset. Skript používá pouze `apt` a nainstaluje
překladač, Ninja a vývojové knihovny potřebné pro SDL3:

```sh
git clone https://github.com/omikron88/Vzlet-Z256.git
cd Vzlet-Z256
sudo ./scripts/install-linux-deps.sh
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
./build/linux-release/vz256 --resources .
```

SDL3, Z80 a Zeta se stáhnou v připnutých verzích při konfiguraci, takže není nutný
systémový balíček SDL3. Stejný postup funguje na 64bitovém Raspberry Pi OS. Na
32bitovém systému lze projekt rovněž sestavit nativně; kvůli delšímu překladu SDL3
je vhodné ponechat paralelismus presetu omezený na dvě úlohy.

Pro server, CI nebo Raspberry Pi bez grafického prostředí lze sestavit pouze jádro:

```sh
cmake --preset linux-core
cmake --build --preset linux-core
ctest --preset linux-core
```

GitHub Actions ověřuje plné sestavení, všechny testy a headless spuštění SDL aplikace
na Ubuntu 24.04 pro amd64 i arm64. ARM64 sestavení používá stejné ABI a závislosti
jako 64bitový Raspberry Pi OS.

### Statický balíček

Preset `linux-static` vloží monitorovou i znakovou ROM přímo do programu a staticky
přilinkuje SDL3, Z80, Zeta a při použití GCC také jeho C++ runtime. Dynamické
zůstávají pouze systémové knihovny Linuxu
(například `libc`, grafický ovladač a knihovny X11/Wayland načítané systémem):

```sh
cmake --preset linux-static
cmake --build --preset linux-static
ctest --preset linux-static
./build/linux-static/vz256
```

Takto vytvořený program pro spuštění nepotřebuje adresář `roms`, parametr
`--resources` ani samostatný bootovací obraz. Statická varianta obsahuje také výchozí
bootovací disk, ze kterého CP/M nabootuje v režimu pouze pro čtení. Pokud je program
spuštěn v kořeni projektu, může použít zapisovatelný `disks/boot.img`; jiný zapisovatelný
obraz lze zadat parametrem `--drive-a` nebo připojit přes dialog mechanik.

#### Windows x64

Na Windows lze ze „Developer PowerShell for VS 2022“ vytvořit jediný přenositelný
`vz256.exe` pomocí stejné statické varianty:

```powershell
cmake --preset windows-static
cmake --build --preset windows-static
ctest --preset windows-static
.\build\windows-static\Release\vz256.exe
```

Program obsahuje monitorovou ROM, znakovou ROM, výchozí bootovací disk, SDL3, Z80,
Zeta a statický MSVC runtime (`/MT`). Nepotřebuje tedy DLL těchto knihoven ani další
datové soubory. Nadále používá standardní systémové DLL dodávané s Windows. Vlastní
zapisovatelné obrazy disket zůstávají volitelnými externími soubory.

Obrazy a geometrie všech mechanik lze zadat samostatně (písmena `a` až `d`):

```sh
./build/vz256 --resources . \
  --drive-a disks/boot.img --geometry-a 5.25-dsdd-80 \
  --drive-c disks/system8.img --geometry-c 8-dssd-77 --read-only-c
```

Známé profily jsou `5.25-dsdd-80`, `5.25-dsdd-40`, `8-sssd-77`, `8-dssd-77`
a `8-dsdd-77`. Poslední profil používá 77 stop, dvě strany a 26 sektorů po
256 bajtech. Bez parametru `--geometry-X` se profil jednoznačně rozpozná podle
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

1. WD2797: doplnit Read/Write Track, CRC a reálné rotační časování.
2. Z80 PIO/FDC CTC/SIO: režimy, vektory IM2 a dva prioritní řetězce podle specifikace.
3. MC6845: odvozovat přesné horizontální a vertikální časování snímku z registrů.
4. TCP sériové linky, tisk do souboru, magnetofonní WAV a debugger CPU/paměti.

ROM se nikdy nemění. Všechny připojené zapisovatelné diskové obrazy se při ukončení
uloží pouze tehdy, pokud byly změněny.
