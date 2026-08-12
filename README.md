# Emulátor Vzlet Z-256

Základ přenositelného emulátoru československého počítače Vzlet Z-256. Modeluje
256 KiB stránkované RAM, překryv boot ROM a pracovní SRAM, 128 KiB videopaměti,
klávesnici, registry periferií a sektorové operace WD2797. SDL3 zobrazuje
čtyřstupňový obraz 640×300 ve správném poměru stran.

## Sestavení

```sh
cmake -S . -B build
cmake --build build
./build/vz256                         # použije přiloženou ROM a obraz disku
./build/vz256 moje.rom muj-disk.img   # vlastní média
```

SDL3 je volitelný pouze proto, aby šlo jádro testovat i na serveru bez grafiky:
`cmake -S . -B build -DVZ256_WITH_SDL=OFF`. F12 provede reset. Kurzorové klávesy,
Home, Delete (ROL), Insert (COPY), End (BREAK) a F1–F3 odpovídají historické
klávesnici.

## Stav a zapojení CPU

Rozhraní stroje (`read`, `write`, `in`, `out`) je připravené jako sběrnice pro
knihovnu [redcode/Z80](https://github.com/redcode/Z80). Aktuální revize dokončuje
model desek a SDL front-end; `run_cycles` zatím hodinami posouvá periferie a před
prvním použitelným CP/M startem je nutné připojit callbacky konkrétní revize Z80.
Toto omezení je záměrně uvedeno otevřeně, místo aby aplikace předstírala běh CPU.

Geometrie přiloženého obrazu je 80 stop × 2 strany × 18 sektorů × 256 bajtů.
Zápis je v API možný jen při připojení s `writable=true`; front-end média otevírá
bezpečně pouze pro čtení.
