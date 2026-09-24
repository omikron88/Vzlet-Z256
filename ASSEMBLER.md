# Jednoduchý assembler 6809/6309

`assembler.py` je dvouprůchodový assembler bez externích závislostí. Tabulku
mnemonik, opcodů a adresovacích režimů nepřepisuje: používá přímo veřejná data
z `ins6809.py`. Výchozí cílový procesor je Hitachi 6309; přepínačem `--cpu
6809` se instrukce a registry dostupné pouze na 6309 odmítnou.

## Použití

```sh
python3 assembler.py program.asm -o program.bin
python3 assembler.py program.asm -o program.hex --format hex --cpu 6809
python3 assembler.py program.asm -o oblast.bin --trim --fill 0xff
```

Binární výstup má standardně přesně 65536 bajtů a mezery vyplňuje hodnotou z
`--fill` (výchozí je nula). `--trim` uloží pouze souvislý úsek od nejnižší do
nejvyšší použité adresy. Intel HEX obsahuje jen skutečně použité oblasti, takže
nesouvislé bloky nevytvářejí zbytečná data.

## Syntaxe

* návěští lze psát s dvojtečkou, `konstanta EQU výraz` také bez ní;
* čísla: `$CAFE`, `0xCAFE`, `%1010`, desetinná čísla a znakové konstanty;
* výrazy podporují `+ - * // % << >> & | ^ ~` a `*` jako aktuální adresu;
* `<výraz` vynutí direct, `>výraz` extended adresování;
* podporováno je immediate, direct, extended, relativní a úplné běžné
  indexované adresování 6809 včetně nepřímého a PCR; rovněž W formy 6309;
* registry pro `TFR`, `EXG`, `PSH*`, `PUL*`, `TFM` a bitové instrukce 6309 se
  převádějí na příslušný postbyte.

Direktivy jsou `ORG`, `EQU`/`SET`, `DB`/`FCB`/`BYTE`, `DW`/`FDB`/`WORD`,
`RMB`/`DS`, `FILL hodnota,počet` a `END`. Řetězec v `DB` se zapisuje v Latin-1.

```asm
        org $F000
start:  ldx #text
loop:   lda ,x+
        beq hotovo
        sta $0400
        bra loop
hotovo: rts
text:   db "Ahoj!", 0
vektor: dw start
```

Při automatické volbě se adresa `$00..$FF` zakóduje jako direct, ostatní jako
extended. U dopředného odkazu proběhne opakované ustálení rozložení; tam, kde
má mít symbol záměrně direct režim, je přesto nejčitelnější použít `<symbol`.
