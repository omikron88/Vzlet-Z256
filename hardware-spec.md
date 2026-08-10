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
- Disky: Dvě 5.25palcové mechaniky 80 stop, double side, double density,
         dvě 8palcové mechaniky, 77 stop, single side, single density
