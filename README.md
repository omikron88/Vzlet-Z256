# Emulátor Vzlet Z-256

Přenosný základ emulátoru historického počítače Vzlet Z-256. Projekt používá C++20,
SDL 3 pro okno, vstup a výstup obrazu a procesorové jádro
[redcode/Z80](https://github.com/redcode/Z80). Zapojení vychází z
[`hardware-spec.md`](hardware-spec.md) a dobových podkladů v adresáři [`doc`](doc).

## Co je implementováno

- 256 KiB operační RAM a nezávislé stránky pro čtení, zápis a M1 fetch (`0xFC`);
  při studeném startu je první 64KiB banka nulová a zbývající tři banky jsou
  vyplněny `0xE5` jako prázdný CP/M RAM disk, zatímco běžný reset jejich obsah zachová,
- monitorová EPROM/SRAM v prvních 16 KiB včetně zrcadlení kratší EPROM,
- sekundární stránkování `0xC0–0xCF`, 128 KiB planární VRAM a paměťové registry MC6845,
- převod dvou bitových rovin na 640×300 ve čtyřech odstínech šedi, včetně
  adresního prokládání MA/RA a počáteční adresy obrazu řízené MC6845,
- hardwarový kurzor MC6845 řízený registry R10/R11/R14/R15, včetně vypnutí,
  obtékajícího rozsahu rastrových řádků a režimů blikání po 16 nebo 32 snímcích,
- délka snímku MC6845 odvozená z horizontálního součtu R0, vertikálního
  součtu R4, korekce R5 a výšky znaku R9 při skutečném 12,5MHz pixel clocku;
  paprsek generuje display-enable a horizontální i vertikální synchronizační
  pulzy podle R1/R2/R3/R6/R7 pro skládání kompozitního obrazového signálu; HSync
  ani VSync nejsou připojeny do přerušovacího řetězce,
- obrazy disket s konfigurovatelnou geometrií a sektory 128, 256 nebo 512 bajtů,
- čtení i atomický zápis obrazů ImageDisk (`.imd`) s nestandardními čísly
  sektorů, komprimovanými datovými poli, velikostmi 128–8192 bajtů, tabulkou
  velikostí `0xff`, smíšenými velikostmi sektorů, smazanými datovými značkami
  a CRC chybami,
- paralelní aktivně nízká ASCII klávesnice přes PIO A, ASTB přerušení, fronta znaků
  a SDL mapování Ctrl, kurzorových a speciálních kláves,
- WD2797 s příkazy Restore/Seek/Step, Read/Write Sector, multi-sector přenosem,
  Read Address, Read/Write Track, Force Interrupt a signály DRQ/INTRQ přes Z80 PIO B;
  čtená stopa obsahuje správné CRC-16 pro FM i MFM a Write Track rozpoznává
  formátovací značky WD2797, ID pole a datová pole pro bezpečné formátování média,
- Type I příkazy udržují fyzickou polohu hlavy odděleně od track registru,
  respektují příznak update, obyčejný Step opakuje naposledy zvolený směr a
  volba verify hlásí seek error při neshodě registru, hlavy nebo geometrie média;
  stav typu I rovněž odráží příkazem řízené naložení hlavy a ochranu média,
- příkazy Restore/Seek/Step zůstávají BUSY po zvolenou dobu kroku 3–15 ms pro
  8″ nebo 6–30 ms pro 5,25″ mechaniky a teprve poté aktivují INTRQ; Force
  Interrupt rozpracovaný pohyb okamžitě ukončí, zruší časovač a zachová původní
  fyzickou polohu i registr stopy; další Type I příkaz je během BUSY ignorován,
- časování datových přenosů WD2797 včetně chyby LOST DATA, pokud procesor
  neobslouží DRQ před příchodem následujícího bajtu; datová perioda respektuje
  1MHz hodinový vstup 5,25″ mechanik a 2MHz vstup 8″ mechanik i režim FM/MFM,
- potlačení opakovaného PIO mode-3 přerušení po dobu obsluhy téhož požadavku;
  stále aktivní DRQ tak před instrukcí INI/OUTI nevytvoří falešný přenos navíc,
- rotační fáze mechaniky a časově řízený čtyřmilisekundový index pulz při
  300 ot./min pro 5,25″ a 360 ot./min pro 8″ média, včetně podmíněného
  přerušení Force Interrupt při každém index pulzu,
- rotační latence Read/Write Sector a geometricky odvozené mezery mezi sektory
  namísto okamžitého zpřístupnění dat po vydání příkazu,
- head-load příznak typu II/III s prodlevou 30 ms pro 5,25″ a 15 ms pro 8″
  mechaniky, po které řadič znovu vyhledá úhlovou pozici požadovaného sektoru,
- Read Address čeká na nejbližší fyzické ID pole podle úhlové polohy média,
  vrací jeho skutečné číslo sektoru a načte je také do sector registru WD2797,
- podmíněná přerušení Force Interrupt při přechodech READY↔NOT READY po vložení
  nebo vysunutí média ve vybrané mechanice,
- samostatná fyzická poloha hlavy každé ze čtyř mechanik; společný track registr
  WD2797 lze při střídání zdrojové a cílové diskety obnovovat nezávisle,
- časový limit diskových operací přes kanál 3 CPU CTC, takže přístup k prázdné
  mechanice skončí chybou BIOSu namísto trvalého čekání,
- čtyřkanálový CPU Z80 CTC v režimu timer/counter, prescalery 16/256, čtení
  čítače, kaskáda kanálů 2→3 a vektorovaná přerušení IM2,
- druhý čtyřkanálový CTC na FDC desce na portech `0xD8–0xDB`, taktovaný 1 MHz,
  včetně samostatných IM2 vektorů; kanály 1/2 čekají na budoucí vstup z kazety,
- Centronics tiskárna přes CPU PIO na portech `0xF0–0xF3`; parametr
  `--printer FILE` ukládá výstup CP/M `LIST` do zvoleného souboru,
- dvoukanálový CPU Z80 SIO na portech `0xF8–0xFB` s programováním registrů,
  jehož kanály lze volbami `--serial-a PORT` a `--serial-b PORT` připojit jako
  binární TCP servery na loopbacku `127.0.0.1`; spojení je neblokující a po
  odpojení klienta přijme nové spojení; alternativní volby
  `--serial-a-connect HOST:PORT` a `--serial-b-connect HOST:PORT` vytvoří
  neblokujícího klienta, který spojení po výpadku automaticky obnovuje,
  přijímacími frontami, vysílacím výstupem a prioritními IM2 přerušeními
  při přijetí znaku i vyprázdnění vysílače; volba WR1
  Status Affects Vector rozlišuje ve vektoru zdroj i kanál přerušení,
- druhý Z80 SIO na FDC desce na portech `0xDC–0xDF`, se samostatnými kanály,
  datovými frontami a přerušením za FDC PIO a CTC v prioritním řetězci,
- obecný model Z80 PIO s režimy 0–3, směrovou a přerušovací maskou režimu 3,
  AND/OR úrovňovou podmínkou, STROBE přerušením a prioritou kanálu A; používá
  jej CPU deska pro tiskárnu i FDC deska pro klávesnici a signály WD2797,
- stav interrupt-in-service pro CTC, PIO a SIO, blokování níže prioritních
  zařízení v daisy-chainu a uvolnění řetězce skutečnou instrukcí Z80 `RETI`,
- přehrávání kazet z PCM WAV (8/16 bit, mono/stereo), převod vzorků na hrany
  komparátoru a přivedení levého kanálu na vstupy 1/2 FDC CTC; `--tape FILE.wav`,
- pokud cesta předaná přes `--tape` neexistuje, vytvoří se prázdný stereo PCM WAV
  s 16bitovými vzorky při 44,1 kHz, připravený pro budoucí záznam z emulátoru,
- RTS kanálu B FDC SIO přepíná sériový/kazetový vstup a při zvolené kazetě
  neaktivní RTS kanálu A spouští motor stejně jako relé skutečného počítače,
- bajty vyslané FDC SIO do nové kazety se zapisují jako stereo 16bit PCM;
  přenosová rychlost se odvozuje od 1MHz vstupu FDC CTC 0, jeho prescaleru a
  konstanty, hardwarové děličky dvěma a násobiče x1/x16/x32/x64 v SIO WR4;
  rámec používá fázové dvojice `10` pro nulu a `01` pro jedničku,
- přehrávač vzorkuje obě stopy ve stejné naprogramované rychlosti, dekóduje
  fázové dvojice, start/data/stop bity a předává přijaté bajty do FDC SIO A/B,
- při načtení se každý WAV kanál samostatně zbaví stejnosměrného posunu a
  normalizuje na jednotnou úroveň, takže fungují i tiché nebo nestejně hlasité pásky,
- vstupní komparátory používají Schmittovu hysterezi, která potlačuje falešné hrany
  od šumu v okolí nulové úrovně,
- kazetový dekodér na každé spolehlivé hraně znovu srovná fázi půlbitových oken,
  z intervalů mezi hranami průběžně odhaduje skutečnou délku půlbitu a kompenzuje
  tak kolísání rychlosti pásku i dlouhodobě roztažené nahrávky,
- pokud CTC ještě neposkytuje platné hodiny, dekodér odhadne počáteční přenosovou
  rychlost přímo z nejkratší skupiny intervalů mezi hranami WAV signálu a zkusí
  obě polarity; vybere tu, která poskytne více platných asynchronních rámců,
- grafické rozhraní Dear ImGui s hlavním menu, stavem mechanik, pozastavením,
  fullscreen režimem, správou diskových obrazů a kazetovým transportem přes
  nativní dialogy SDL3; samostatné dceřiné SDL okno kazety se otevírá klávesou
  **F9** a nabízí časovou
  osu, skoky o deset sekund, přesun na začátek nebo konec pásku a volitelný
  stereo zvukový monitor přehrávaný přes SDL3; rychlost přehrávání lze měnit
  v rozsahu 0,25× až 4× a dva živé ukazatele zobrazují špičkovou úroveň stop,
- samostatné dceřiné SDL okno sériových linek otevřené klávesou **F8** umožňuje
  za běhu pro každý CPU SIO kanál zvolit vypnutí, TCP naslouchání nebo připojení,
  změnit hostitele a port a sledovat stav spojení i počty přenesených bajtů,
- debugger otevřený klávesou **F11** běží v samostatném dceřiném SDL okně,
  automaticky pozastaví stroj a nabízí spuštění, krok do instrukce, krok přes
  volání, reset, disassembler, editovatelné registry a paměť, zásobník a
  breakpointy nastavované kliknutím na řádek disassembleru,
- rozměry a poloha oken debuggeru, mechanik, kazety, sériových linek, klávesových
  zkratek a informací o programu se ukládají do uživatelského adresáře aplikace
  a obnoví se při jejich příštím otevření,
- nabídka **Help > About Vzlet Z-256** otevírá samostatné informační okno s číslem
  verze, UTC datem sestavení, použitými knihovnami a odkazem na zdrojový projekt,
- položka **Control > Keyboard shortcuts…** umožňuje změnit funkční klávesy pro
  pauzu, reset a otevření debuggeru, mechanik, kazety a sériových linek; nastavení
  se ukládá do `shortcuts.cfg` ve stejném uživatelském adresáři jako polohy oken,
- volby poměru stran, celočíselného škálování, scanlines a celé obrazovky z nabídky
  **View** se ukládají do `view.cfg`; poloha a velikost hlavního okna se ukládá do
  `main.window` a při ukončení v celoobrazovkovém režimu se nepřepisuje,
- položky **File > Export configuration…** a **Import configuration…** přenesou
  všechny uložené uživatelské volby a rozložení oken v jediném souboru `.vzconfig`;
  emulátor se po zvolení souboru ukončí, aby export obsahoval právě uložený stav a
  importované nastavení nebylo při ukončování přepsáno; výsledek operace se při
  příštím spuštění zobrazí po dobu deseti sekund ve stavovém řádku,
- okna mechanik a kazety udržují deset naposledy úspěšně otevřených nebo
  vytvořených médií; položku lze jedním kliknutím znovu připojit, chybějící
  soubory jsou označeny a seznam lze po jednotlivých položkách nebo celý vymazat;
  seznam se ukládá do `recent-media.cfg` a je součástí exportu konfigurace,
- okno **File > Configuration profiles…** ukládá stejné nastavení do pojmenovaných
  profilů, umožňuje je načítat nebo mazat a dovoluje tak udržovat několik pracovních
  rozložení; přepsání, načtení a odstranění profilu vyžaduje potvrzení a názvy mohou
  obsahovat písmena, číslice, mezery, pomlčky a podtržítka,
- adaptér procesoru redcode/Z80, real-time smyčka na 4 MHz, reset klávesou **F12**
  a automatické načtení dodaných ROM/disku.

Klávesa **F10** nebo položka **File > Disk drives** otevře samostatné dceřiné SDL
okno pro grafickou správu čtyř mechanik B–E. Disk A je v CP/M zabudovaný RAM disk.
Okno umožňuje vybrat geometrii, otevřít existující obraz, uložit jej,
vysunout médium a přepínat ochranu proti zápisu. Tlačítko **Create blank…** vytvoří
podle právě zvolené geometrie nový obraz, vyplní jej
bajtem `0xE5` používaným CP/M pro volné adresářové položky a rovnou jej připojí do
vybrané mechaniky. U změněného obrazu je před vysunutím nutné změny uložit nebo
výslovně zahodit; přepsání existujícího obrazu při vytváření prázdné diskety také
vyžaduje potvrzení. Zápis obrazu používá dočasný soubor a atomické přejmenování.

Hlavní menu nabízí reset (**F12**), pozastavení emulace (**F5**) a fullscreen
režim (**Alt+Enter**). Dear ImGui je staženo v připnuté verzi a přilinkováno
staticky k frontendu; stažení lze řídit volbou `VZ256_FETCH_IMGUI`, případně
použít lokální checkout přes `VZ256_IMGUI_SOURCE_DIR`.
V nabídce **View** lze zachovat poměr obrazu 4:3, zapnout celočíselné škálování
a volitelné scanlines. Stavový pruh je ukotvený u spodního okraje okna mimo
plochu emulované obrazovky. Obrazovka respektuje prostor obsazený hlavním menu
i stavovým pruhem, takže ovládací prvky nikdy nepřekrývají emulovaný obraz.

PIO a SIO na FDC desce jsou zapojeny do společného prioritního daisy-chainu s
CPU periferiemi. Modely lze spolu s paměťovým a obrazovým jádrem testovat bez SDL.

## Sestavení

Je třeba CMake 3.24+ a překladač s C++20. CMake stáhne připnuté revize SDL 3,
`redcode/Z80` a jeho závislosti Zeta. Systémové SDL3 použije automaticky;
v prostředí bez sítě lze předat již existující checkout Z80 a vypnout stažení SDL:

```sh
cmake -S . -B build -DVZ256_Z80_SOURCE_DIR=/cesta/k/Z80 -DVZ256_FETCH_SDL3=OFF
cmake --build build
./build/vz256 --resources .
```

Výstup na tiskárnu lze připojit k souboru, do kterého se jednotlivé tiskové úlohy
přidávají i při dalších spuštěních emulátoru:

```sh
./build/vz256 --resources . --printer print.txt
./build/vz256 --resources . --serial-a 8000 --serial-b 8001
# druhý emulátor propojený kanálem A s prvním:
./build/vz256 --resources . --serial-a-connect 127.0.0.1:8000
# spuštění rovnou s otevřeným a pozastaveným debuggerem:
./build/vz256 --resources . --debugger
```

Výstupní soubor lze vybrat nebo změnit také za běhu pomocí položky
**File > Printer output…**. Položka **Disconnect printer** tiskárnu od souboru
odpojí; aktuální stav a název souboru se zobrazují ve spodním stavovém pruhu.
Při diskové operaci stavový pruh navíc ukazuje vybranou mechaniku, stopu, stranu,
sektor, druh operace a aktivní signály `DRQ`/`INTRQ` řadiče WD2797.
Čtyři LED označené B–E svítí zeleně při čtení nebo vyhledávání a červeně při
zápisu či formátování. Krátké operace zůstávají indikovány ještě 150 ms, aby byla
aktivita viditelná i na rychlém hostitelském počítači.

Pro obvyklé kazetové rychlosti se FDC CTC 0 nastaví do counter režimu řídicím
slovem `0x47`; následuje časová konstanta. Za hardwarovou děličkou dvěma jsou
nejbližší nastavení při vstupu 1 MHz tato:

| Požadováno | CTC konstanta | SIO WR4 | Skutečná rychlost | Odchylka |
|---:|---:|---:|---:|---:|
| 1200 Bd | 26 | `0x44` (x16) | 1201,92 Bd | +0,16 % |
| 2400 Bd | 208 | `0x04` (x1) | 2403,85 Bd | +0,16 % |
| 4800 Bd | 104 | `0x04` (x1) | 4807,69 Bd | +0,16 % |
| 9600 Bd | 52 | `0x04` (x1) | 9615,38 Bd | +0,16 % |

Registr WR4 se zapisuje tak, že se na řídicí port FDC SIO (`0xDD` pro A,
`0xDF` pro B) nejprve pošle hodnota `4` a potom uvedené řídicí slovo.

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
`--resources`. Bootovací disk není součástí programu. Pokud je program spuštěn
v kořeni projektu, automaticky připojí `disks/boot.img`; jiný obraz lze zadat
parametrem `--drive-b` nebo připojit přes dialog mechanik. Bez externího obrazu
zůstane mechanika B prázdná.

### Windows / MSYS2

Nainstalujte [MSYS2](https://www.msys2.org/) a otevřete terminál **MSYS2 UCRT64**.
Aktualizujte jej podle pokynů MSYS2 a pak spusťte:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL3
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVZ256_STATIC_BUNDLE=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/vz256.exe
```

Program obsahuje monitorovou ROM, znakovou ROM, SDL3, Z80, Zeta a statický MSVC
runtime (`/MT`). Nepotřebuje tedy DLL těchto knihoven ani externí ROM soubory.
Nadále používá standardní systémové DLL dodávané s Windows. Obrazy disket včetně
bootovacího disku se připojují z externích souborů.

Obrazy a geometrie všech mechanik lze zadat samostatně (písmena `b` až `e`):

```sh
./build/vz256 --resources . \
  --drive-b disks/boot.img --geometry-b 5.25-dsdd-80 \
  --drive-d disks/system8.img --geometry-d 8-dssd-77 --read-only-d
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

Test `tcp_serial` používá systémem přidělený volný loopback port, propojí režimy
Listen a Connect, přenese binární data oběma směry a po odpojení ověří nové
připojení druhého klienta. Nepoužívá pevný port, a proto může bezpečně běžet
souběžně s ostatními testy i dalšími instancemi sestavení.

Test `boot_disk` spustí dodanou monitorovou EPROM, provede inicializaci periferií a
ověří, že BIOS přes přerušení PIO/WD2797 načte z `boot.img` jak diskový loader do
`0xF000`, tak CCP/BDOS do `0xE400`, vypíše prompt CP/M a přejde do čekání na
klávesnici v rutině BIOS CONIN. Následně přes PIO klávesnici zadá `DIR` a ověří
návrat na další prompt. Test pak připojí druhou prázdnou disketu, načte soubor ze
zdrojové mechaniky a několikrát mezi oběma mechanikami přepne, čímž regresně
ověřuje skutečnou cestu BIOSu používanou při kopírování disketa–disketa.

ROM se nikdy nemění. Všechny připojené zapisovatelné diskové obrazy se při ukončení
uloží pouze tehdy, pokud byly změněny.
