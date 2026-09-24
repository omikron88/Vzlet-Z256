"""Motorola 6809 / Hitachi 6309 instruction table.

Transcribed from Darren Atkinson's *Motorola 6809 and Hitachi 6309
Programmers Reference* (2009), chiefly the Programming Aid on pages 146-150.

One :class:`Instruction` represents one concrete opcode/addressing-mode form.
Opcode prefixes are stored as real bytes (for example ``b"\x10\x89"``), not
as a packed integer.  Cycle and byte counts remain strings because the source
contains expressions such as ``"6+"``, ``"6+3n"`` and conditional timings.

The 6809 and 6309-emulation timings are normally identical, but are kept in
separate fields because conditional long branches are a documented exception.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from enum import Enum, auto
from types import MappingProxyType
from typing import Iterable, Mapping


class AddrMode(Enum):
    INHERENT = auto()
    IMMEDIATE = auto()
    DIRECT = auto()
    INDEXED = auto()
    EXTENDED = auto()
    RELATIVE = auto()


class OperandType(Enum):
    NONE = auto()
    IMM8 = auto()
    IMM16 = auto()
    IMM32 = auto()
    DIRECT = auto()
    INDEXED = auto()
    EXTENDED = auto()
    REL8 = auto()
    REL16 = auto()
    REG_PAIR = auto()          # TFR/EXG and 6309 inter-register arithmetic
    REG_LIST = auto()          # PSH/PUL postbyte
    TFM_REG_PAIR = auto()      # restricted TFM register pair postbyte
    BIT_TRANSFER = auto()      # 6309 bit-operation postbyte + direct address
    IMM8_DIRECT = auto()       # AIM/OIM/EIM/TIM: mask + direct address
    IMM8_INDEXED = auto()      # AIM/OIM/EIM/TIM: mask + indexed postbyte
    IMM8_EXTENDED = auto()     # AIM/OIM/EIM/TIM: mask + 16-bit address


@dataclass(frozen=True, slots=True)
class Instruction:
    mnemonic: str
    opcode: bytes
    mode: AddrMode
    operand: OperandType
    cycles_6809: str | None
    cycles_6309_emulation: str
    cycles_6309_native: str
    byte_count: str
    only_6309: bool = False
    aliases: tuple[str, ...] = ()
    note: str | None = None

    @property
    def minimum_byte_count(self) -> int:
        """Minimum encoded length; indexed forms may require extra bytes."""
        digits = "".join(ch for ch in self.byte_count if ch.isdigit())
        if not digits:
            raise ValueError(f"No numeric byte count in {self.byte_count!r}")
        return int(digits)


@dataclass(frozen=True, slots=True)
class IndexedForm:
    """One indexed-addressing postbyte form from page 150.

    ``cycles_*`` and ``extra_bytes`` are added to the base values ending in
    ``+`` in an :class:`Instruction` record.
    """

    assembler_form: str
    postbyte_pattern: str
    indirect: bool
    cycles_6809: str | None
    cycles_6309_emulation: str
    cycles_6309_native: str
    extra_bytes: int
    only_6309: bool = False
    description: str = ""


def _opcode(text: str) -> bytes:
    """Decode the compact notation used by the Programming Aid.

    ``0xx`` means page-2 prefix $10; ``1xx`` means page-3 prefix $11.
    """
    text = text.upper()
    if len(text) == 2:
        return bytes.fromhex(text)
    if len(text) == 3 and text[0] in "01":
        return bytes((0x10 + int(text[0]), int(text[1:], 16)))
    raise ValueError(f"Unsupported compact opcode: {text!r}")


def _split_cycles(source: str) -> tuple[str, str]:
    """Return (emulation, native) cycle expressions from the source notation."""
    source = source.replace(" ", "")
    if "/" not in source:
        return source, source
    emulation, native = source.split("/", 1)
    # In the source a trailing '+' applies to both halves (for example 7/6+).
    if native.endswith("+") and not emulation.endswith("+"):
        emulation += "+"
    return emulation, native


_instructions: list[Instruction] = []


def _add(
    mnemonic: str,
    opcode: str,
    mode: AddrMode,
    operand: OperandType,
    cycles: str,
    byte_count: str,
    *,
    only_6309: bool = False,
    aliases: tuple[str, ...] = (),
    note: str | None = None,
    cycles_6809: str | None = None,
    cycles_6309_emulation: str | None = None,
    cycles_6309_native: str | None = None,
) -> None:
    emulation, native = _split_cycles(cycles)
    if cycles_6809 is None and not only_6309:
        cycles_6809 = emulation
    if cycles_6309_emulation is None:
        cycles_6309_emulation = emulation
    if cycles_6309_native is None:
        cycles_6309_native = native
    _instructions.append(
        Instruction(
            mnemonic=mnemonic,
            opcode=_opcode(opcode),
            mode=mode,
            operand=operand,
            cycles_6809=cycles_6809,
            cycles_6309_emulation=cycles_6309_emulation,
            cycles_6309_native=cycles_6309_native,
            byte_count=byte_count,
            only_6309=only_6309,
            aliases=aliases,
            note=note,
        )
    )


def _memory_forms(
    mnemonic: str,
    immediate_type: OperandType | None,
    specs: Mapping[AddrMode, tuple[str, str, str]],
    *,
    only_6309: bool = False,
) -> None:
    operands = {
        AddrMode.IMMEDIATE: immediate_type,
        AddrMode.DIRECT: OperandType.DIRECT,
        AddrMode.INDEXED: OperandType.INDEXED,
        AddrMode.EXTENDED: OperandType.EXTENDED,
    }
    for mode, (opcode, cycles, size) in specs.items():
        operand = operands[mode]
        if operand is None:
            raise ValueError(f"Immediate type missing for {mnemonic}")
        _add(mnemonic, opcode, mode, operand, cycles, size, only_6309=only_6309)


def _inherent(
    mnemonic: str,
    opcode: str,
    cycles: str,
    size: str,
    *,
    only_6309: bool = False,
    aliases: tuple[str, ...] = (),
    note: str | None = None,
) -> None:
    _add(
        mnemonic, opcode, AddrMode.INHERENT, OperandType.NONE, cycles, size,
        only_6309=only_6309, aliases=aliases, note=note,
    )


# ---------------------------------------------------------------------------
# Instruction definitions
# ---------------------------------------------------------------------------

_inherent("ABX", "3A", "3/1", "1")

for name, codes in {
    "ADCA": ("89", "99", "A9", "B9"),
    "ADCB": ("C9", "D9", "E9", "F9"),
}.items():
    _memory_forms(name, OperandType.IMM8, dict(zip(
        (AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED),
        zip(codes, ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3")),
    )))
_memory_forms("ADCD", OperandType.IMM16, {
    AddrMode.IMMEDIATE: ("089", "5/4", "4"), AddrMode.DIRECT: ("099", "7/5", "3"),
    AddrMode.INDEXED: ("0A9", "7/6+", "3+"), AddrMode.EXTENDED: ("0B9", "8/6", "4"),
}, only_6309=True)
_add("ADCR", "031", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)

for name, imm, codes, cycles, sizes, only in (
    ("ADDA", OperandType.IMM8, ("8B", "9B", "AB", "BB"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("ADDB", OperandType.IMM8, ("CB", "DB", "EB", "FB"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("ADDD", OperandType.IMM16, ("C3", "D3", "E3", "F3"), ("4/3", "6/4", "6/5+", "7/5"), ("3", "2", "2+", "3"), False),
    ("ADDE", OperandType.IMM8, ("18B", "19B", "1AB", "1BB"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("ADDF", OperandType.IMM8, ("1CB", "1DB", "1EB", "1FB"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("ADDW", OperandType.IMM16, ("08B", "09B", "0AB", "0BB"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {
        mode: spec for mode, spec in zip(
            (AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED),
            zip(codes, cycles, sizes),
        )
    }, only_6309=only)
_add("ADDR", "030", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)

def _masked_memory(name: str, codes: tuple[str, str, str]) -> None:
    for mode, operand, code, cycles, size in zip(
        (AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED),
        (OperandType.IMM8_DIRECT, OperandType.IMM8_INDEXED, OperandType.IMM8_EXTENDED),
        codes, ("6", "7+", "7"), ("3", "3+", "4"),
    ):
        _add(name, code, mode, operand, cycles, size, only_6309=True)

_masked_memory("AIM", ("02", "62", "72"))

for name, imm, codes, cycles, sizes, only in (
    ("ANDA", OperandType.IMM8, ("84", "94", "A4", "B4"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("ANDB", OperandType.IMM8, ("C4", "D4", "E4", "F4"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("ANDD", OperandType.IMM16, ("084", "094", "0A4", "0B4"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {m: s for m, s in zip(
        (AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes)
    )}, only_6309=only)
_add("ANDCC", "1C", AddrMode.IMMEDIATE, OperandType.IMM8, "3", "2")
_add("ANDR", "034", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)

for name, opcode, cycles, size, aliases, only in (
    ("ASLA", "48", "2/1", "1", ("LSLA",), False),
    ("ASLB", "58", "2/1", "1", ("LSLB",), False),
    ("ASLD", "048", "3/2", "2", ("LSLD",), True),
):
    _inherent(name, opcode, cycles, size, only_6309=only, aliases=aliases)
for mode, code, cyc, size in (
    (AddrMode.DIRECT, "08", "6/5", "2"),
    (AddrMode.INDEXED, "68", "6+", "2+"),
    (AddrMode.EXTENDED, "78", "7/6", "3"),
):
    _add("ASL", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size, aliases=("LSL",))

for name, opcode, only in (("ASRA", "47", False), ("ASRB", "57", False), ("ASRD", "047", True)):
    _inherent(name, opcode, "3/2" if name == "ASRD" else "2/1", "2" if name == "ASRD" else "1", only_6309=only)
for mode, code, cyc, size in (
    (AddrMode.DIRECT, "07", "6/5", "2"), (AddrMode.INDEXED, "67", "6+", "2+"),
    (AddrMode.EXTENDED, "77", "7/6", "3"),
):
    _add("ASR", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)

for name, code in (("BAND", "130"), ("BIAND", "131"), ("BOR", "132"), ("BIOR", "133"), ("BEOR", "134"), ("BIEOR", "135")):
    _add(name, code, AddrMode.DIRECT, OperandType.BIT_TRANSFER, "7/6", "4", only_6309=True)

for name, imm, codes, cycles, sizes, only in (
    ("BITA", OperandType.IMM8, ("85", "95", "A5", "B5"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("BITB", OperandType.IMM8, ("C5", "D5", "E5", "F5"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("BITD", OperandType.IMM16, ("085", "095", "0A5", "0B5"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {m: s for m, s in zip(
        (AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes)
    )}, only_6309=only)
_add("BITMD", "13C", AddrMode.IMMEDIATE, OperandType.IMM8, "4", "3", only_6309=True)

# Relative branches.  BHS/BLO and their long forms are accepted as aliases.
for name, code, aliases in (
    ("BRA", "20", ()), ("BRN", "21", ()), ("BHI", "22", ()), ("BLS", "23", ()),
    ("BCC", "24", ("BHS",)), ("BCS", "25", ("BLO",)), ("BNE", "26", ()),
    ("BEQ", "27", ()), ("BVC", "28", ()), ("BVS", "29", ()), ("BPL", "2A", ()),
    ("BMI", "2B", ()), ("BGE", "2C", ()), ("BLT", "2D", ()), ("BGT", "2E", ()),
    ("BLE", "2F", ()),
):
    _add(name, code, AddrMode.RELATIVE, OperandType.REL8, "3", "2", aliases=aliases)
_add("BSR", "8D", AddrMode.RELATIVE, OperandType.REL8, "7/6", "2")
_add("LBRA", "16", AddrMode.RELATIVE, OperandType.REL16, "5/4", "3")
_add("LBSR", "17", AddrMode.RELATIVE, OperandType.REL16, "9/7", "3")
for name, code, aliases in (
    ("LBRN", "021", ()), ("LBHI", "022", ()), ("LBLS", "023", ()),
    ("LBCC", "024", ("LBHS",)), ("LBCS", "025", ("LBLO",)),
    ("LBNE", "026", ()), ("LBEQ", "027", ()), ("LBVC", "028", ()),
    ("LBVS", "029", ()), ("LBPL", "02A", ()), ("LBMI", "02B", ()),
    ("LBGE", "02C", ()), ("LBLT", "02D", ()), ("LBGT", "02E", ()),
    ("LBLE", "02F", ()),
):
    _add(
        name, code, AddrMode.RELATIVE, OperandType.REL16, "5", "4",
        aliases=aliases,
        cycles_6809="5 (6 if taken)", cycles_6309_emulation="5", cycles_6309_native="5",
        note="A taken conditional long branch uses a sixth cycle on the 6809 only.",
    )

for name, opcode, cycles, size, only in (
    ("CLRA", "4F", "2/1", "1", False), ("CLRB", "5F", "2/1", "1", False),
    ("CLRD", "04F", "3/2", "2", True), ("CLRE", "14F", "3/2", "2", True),
    ("CLRF", "15F", "3/2", "2", True), ("CLRW", "05F", "3/2", "2", True),
):
    _inherent(name, opcode, cycles, size, only_6309=only)
for mode, code, cyc, size in ((AddrMode.DIRECT, "0F", "6/5", "2"), (AddrMode.INDEXED, "6F", "6+", "2+"), (AddrMode.EXTENDED, "7F", "7/6", "3")):
    _add("CLR", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)

for name, imm, codes, cycles, sizes, only in (
    ("CMPA", OperandType.IMM8, ("81", "91", "A1", "B1"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("CMPB", OperandType.IMM8, ("C1", "D1", "E1", "F1"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("CMPD", OperandType.IMM16, ("083", "093", "0A3", "0B3"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), False),
    ("CMPE", OperandType.IMM8, ("181", "191", "1A1", "1B1"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("CMPF", OperandType.IMM8, ("1C1", "1D1", "1E1", "1F1"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("CMPS", OperandType.IMM16, ("18C", "19C", "1AC", "1BC"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), False),
    ("CMPU", OperandType.IMM16, ("183", "193", "1A3", "1B3"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), False),
    ("CMPW", OperandType.IMM16, ("081", "091", "0A1", "0B1"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
    ("CMPX", OperandType.IMM16, ("8C", "9C", "AC", "BC"), ("4/3", "6/4", "6/5+", "7/5"), ("3", "2", "2+", "3"), False),
    ("CMPY", OperandType.IMM16, ("08C", "09C", "0AC", "0BC"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), False),
):
    _memory_forms(name, imm, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=only)
_add("CMPR", "037", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)

for stem, base, only in (("COM", "3", False), ("DEC", "A", False), ("INC", "C", False)):
    for suffix, prefix, only_item in (("A", "4", False), ("B", "5", False), ("D", "04", True), ("E", "14", True), ("F", "15", True), ("W", "05", True)):
        code = prefix + base
        _inherent(stem + suffix, code, "3/2" if only_item else "2/1", "2" if only_item else "1", only_6309=only_item)
    for mode, lead, cyc, size in ((AddrMode.DIRECT, "0", "6/5", "2"), (AddrMode.INDEXED, "6", "6+", "2+"), (AddrMode.EXTENDED, "7", "7/6", "3")):
        _add(stem, lead + base, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)

_add("CWAI", "3C", AddrMode.IMMEDIATE, OperandType.IMM8, "22/20", "2")
_inherent("DAA", "19", "2/1", "1")

for name, width, codes, cycles, sizes in (
    ("DIVD", OperandType.IMM8, ("18D", "19D", "1AD", "1BD"), ("25", "27/26", "27+", "28/27"), ("3", "3", "3+", "4")),
    ("DIVQ", OperandType.IMM16, ("18E", "19E", "1AE", "1BE"), ("34", "36/35", "36+", "37/36"), ("4", "3", "3+", "4")),
):
    _memory_forms(name, width, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=True)

_masked_memory("EIM", ("05", "65", "75"))
for name, imm, codes, cycles, sizes, only in (
    ("EORA", OperandType.IMM8, ("88", "98", "A8", "B8"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("EORB", OperandType.IMM8, ("C8", "D8", "E8", "F8"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("EORD", OperandType.IMM16, ("088", "098", "0A8", "0B8"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=only)
_add("EORR", "036", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)
_add("EXG", "1E", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "8/5", "2")

for mode, code, cyc, size in ((AddrMode.DIRECT, "0E", "3/2", "2"), (AddrMode.INDEXED, "6E", "3+", "2+"), (AddrMode.EXTENDED, "7E", "4/3", "3")):
    _add("JMP", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)
for mode, code, cyc, size in ((AddrMode.DIRECT, "9D", "7/6", "2"), (AddrMode.INDEXED, "AD", "7/6+", "2+"), (AddrMode.EXTENDED, "BD", "8/7", "3")):
    _add("JSR", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)

for name, imm, codes, cycles, sizes, only in (
    ("LDA", OperandType.IMM8, ("86", "96", "A6", "B6"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("LDB", OperandType.IMM8, ("C6", "D6", "E6", "F6"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("LDD", OperandType.IMM16, ("CC", "DC", "EC", "FC"), ("3", "5/4", "5+", "6/5"), ("3", "2", "2+", "3"), False),
    ("LDE", OperandType.IMM8, ("186", "196", "1A6", "1B6"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("LDF", OperandType.IMM8, ("1C6", "1D6", "1E6", "1F6"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("LDQ", OperandType.IMM32, ("CD", "0DC", "0EC", "0FC"), ("5", "8/7", "8+", "9/8"), ("5", "3", "3+", "4"), True),
    ("LDS", OperandType.IMM16, ("0CE", "0DE", "0EE", "0FE"), ("4/3", "6/5", "6+", "7/6"), ("4", "3", "3+", "4"), False),
    ("LDU", OperandType.IMM16, ("CE", "DE", "EE", "FE"), ("3", "5/4", "5+", "6/5"), ("3", "2", "2+", "3"), False),
    ("LDW", OperandType.IMM16, ("086", "096", "0A6", "0B6"), ("4", "6/5", "6+", "7/6"), ("4", "3", "3+", "4"), True),
    ("LDX", OperandType.IMM16, ("8E", "9E", "AE", "BE"), ("3", "5/4", "5+", "6/5"), ("3", "2", "2+", "3"), False),
    ("LDY", OperandType.IMM16, ("08E", "09E", "0AE", "0BE"), ("4", "6/5", "6+", "7/6"), ("4", "3", "3+", "4"), False),
):
    _memory_forms(name, imm, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=only)
_add("LDMD", "13D", AddrMode.IMMEDIATE, OperandType.IMM8, "5", "3", only_6309=True)
_add("LDBT", "136", AddrMode.DIRECT, OperandType.BIT_TRANSFER, "7/6", "4", only_6309=True)

for name, code in (("LEAX", "30"), ("LEAY", "31"), ("LEAS", "32"), ("LEAU", "33")):
    _add(name, code, AddrMode.INDEXED, OperandType.INDEXED, "4+", "2+")

for name, opcode, cycles, size, only in (
    ("LSRA", "44", "2/1", "1", False), ("LSRB", "54", "2/1", "1", False),
    ("LSRD", "044", "3/2", "2", True), ("LSRW", "054", "3/2", "2", True),
):
    _inherent(name, opcode, cycles, size, only_6309=only)
for mode, code, cyc, size in ((AddrMode.DIRECT, "04", "6/5", "2"), (AddrMode.INDEXED, "64", "6+", "2+"), (AddrMode.EXTENDED, "74", "7/6", "3")):
    _add("LSR", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)

_inherent("MUL", "3D", "11/10", "1")
_memory_forms("MULD", OperandType.IMM16, {
    AddrMode.IMMEDIATE: ("18F", "28", "4"), AddrMode.DIRECT: ("19F", "30/29", "3"),
    AddrMode.INDEXED: ("1AF", "30+", "3+"), AddrMode.EXTENDED: ("1BF", "31/30", "4"),
}, only_6309=True)

for name, opcode, cycles, size, only in (("NEGA", "40", "2/1", "1", False), ("NEGB", "50", "2/1", "1", False), ("NEGD", "040", "3/2", "2", True)):
    _inherent(name, opcode, cycles, size, only_6309=only)
for mode, code, cyc, size in ((AddrMode.DIRECT, "00", "6/5", "2"), (AddrMode.INDEXED, "60", "6+", "2+"), (AddrMode.EXTENDED, "70", "7/6", "3")):
    _add("NEG", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)
_inherent("NOP", "12", "2/1", "1")
_masked_memory("OIM", ("01", "61", "71"))

for name, imm, codes, cycles, sizes, only in (
    ("ORA", OperandType.IMM8, ("8A", "9A", "AA", "BA"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("ORB", OperandType.IMM8, ("CA", "DA", "EA", "FA"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("ORD", OperandType.IMM16, ("08A", "09A", "0AA", "0BA"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=only)
_add("ORCC", "1A", AddrMode.IMMEDIATE, OperandType.IMM8, "3", "2")
_add("ORR", "035", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)

for name, code in (("PSHS", "34"), ("PSHU", "36"), ("PULS", "35"), ("PULU", "37")):
    _add(name, code, AddrMode.IMMEDIATE, OperandType.REG_LIST, "5/4+", "2", note="Add one cycle per byte pushed or pulled.")
for name, code in (("PSHSW", "038"), ("PULSW", "039"), ("PSHUW", "03A"), ("PULUW", "03B")):
    _inherent(name, code, "6", "2", only_6309=True)

for stem, base in (("ROL", "9"), ("ROR", "6")):
    for suffix, prefix, only in (("A", "4", False), ("B", "5", False), ("D", "04", True), ("W", "05", True)):
        _inherent(stem + suffix, prefix + base, "3/2" if only else "2/1", "2" if only else "1", only_6309=only)
    for mode, lead, cyc, size in ((AddrMode.DIRECT, "0", "6/5", "2"), (AddrMode.INDEXED, "6", "6+", "2+"), (AddrMode.EXTENDED, "7", "7/6", "3")):
        _add(stem, lead + base, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)

_add(
    "RTI", "3B", AddrMode.INHERENT, OperandType.NONE, "15/17", "1",
    cycles_6809="15 if CC.E=1; 6 if CC.E=0",
    cycles_6309_emulation="15 if CC.E=1; 6 if CC.E=0",
    cycles_6309_native="17 if CC.E=1; 6 if CC.E=0",
)
_inherent("RTS", "39", "5/4", "1")

for name, imm, codes, cycles, sizes, only in (
    ("SBCA", OperandType.IMM8, ("82", "92", "A2", "B2"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("SBCB", OperandType.IMM8, ("C2", "D2", "E2", "F2"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("SBCD", OperandType.IMM16, ("082", "092", "0A2", "0B2"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=only)
_add("SBCR", "033", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)
_inherent("SEX", "1D", "2/1", "1")
_inherent("SEXW", "14", "4", "1", only_6309=True)

def _store_forms(name: str, codes: tuple[str, str, str], cycles: tuple[str, str, str], sizes: tuple[str, str, str], *, only_6309: bool = False) -> None:
    for mode, operand, spec in zip(
        (AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED),
        (OperandType.DIRECT, OperandType.INDEXED, OperandType.EXTENDED),
        zip(codes, cycles, sizes),
    ):
        code, cyc, size = spec
        _add(name, code, mode, operand, cyc, size, only_6309=only_6309)

for args in (
    ("STA", ("97", "A7", "B7"), ("4/3", "4+", "5/4"), ("2", "2+", "3"), False),
    ("STB", ("D7", "E7", "F7"), ("4/3", "4+", "5/4"), ("2", "2+", "3"), False),
    ("STD", ("DD", "ED", "FD"), ("5/4", "5+", "6/5"), ("2", "2+", "3"), False),
    ("STE", ("197", "1A7", "1B7"), ("5/4", "5+", "6/5"), ("3", "3+", "4"), True),
    ("STF", ("1D7", "1E7", "1F7"), ("5/4", "5+", "6/5"), ("3", "3+", "4"), True),
    ("STQ", ("0DD", "0ED", "0FD"), ("8/7", "8+", "9/8"), ("3", "3+", "4"), True),
    ("STS", ("0DF", "0EF", "0FF"), ("6/5", "6+", "7/6"), ("3", "3+", "4"), False),
    ("STU", ("DF", "EF", "FF"), ("5/4", "5+", "6/5"), ("2", "2+", "3"), False),
    ("STW", ("097", "0A7", "0B7"), ("6/5", "6+", "7/6"), ("3", "3+", "4"), True),
    ("STX", ("9F", "AF", "BF"), ("5/4", "5+", "6/5"), ("2", "2+", "3"), False),
    ("STY", ("09F", "0AF", "0BF"), ("6/5", "6+", "7/6"), ("3", "3+", "4"), False),
):
    name, codes, cycles, sizes, only = args
    _store_forms(name, codes, cycles, sizes, only_6309=only)
_add("STBT", "137", AddrMode.DIRECT, OperandType.BIT_TRANSFER, "8/7", "4", only_6309=True)

for name, imm, codes, cycles, sizes, only in (
    ("SUBA", OperandType.IMM8, ("80", "90", "A0", "B0"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("SUBB", OperandType.IMM8, ("C0", "D0", "E0", "F0"), ("2", "4/3", "4+", "5/4"), ("2", "2", "2+", "3"), False),
    ("SUBD", OperandType.IMM16, ("83", "93", "A3", "B3"), ("4/3", "6/4", "6/5+", "7/5"), ("3", "2", "2+", "3"), False),
    ("SUBE", OperandType.IMM8, ("180", "190", "1A0", "1B0"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("SUBF", OperandType.IMM8, ("1C0", "1D0", "1E0", "1F0"), ("3", "5/4", "5+", "6/5"), ("3", "3", "3+", "4"), True),
    ("SUBW", OperandType.IMM16, ("080", "090", "0A0", "0B0"), ("5/4", "7/5", "7/6+", "8/6"), ("4", "3", "3+", "4"), True),
):
    _memory_forms(name, imm, {m: s for m, s in zip((AddrMode.IMMEDIATE, AddrMode.DIRECT, AddrMode.INDEXED, AddrMode.EXTENDED), zip(codes, cycles, sizes))}, only_6309=only)
_add("SUBR", "032", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "4", "3", only_6309=True)

_inherent("SWI", "3F", "19/21", "1")
_inherent("SWI2", "03F", "20/22", "2")
_inherent("SWI3", "13F", "20/22", "2")
_add(
    "SYNC", "13", AddrMode.INHERENT, OperandType.NONE, ">=4/>=3", "1",
    note="Cycle count depends on interrupt arrival.",
)

for code, note in (
    ("138", "Source form: TFM r0+,r1+"),
    ("139", "Source form: TFM r0-,r1-"),
    ("13A", "Source form: TFM r0+,r1"),
    ("13B", "Source form: TFM r0,r1+"),
):
    _add("TFM", code, AddrMode.IMMEDIATE, OperandType.TFM_REG_PAIR, "6+3n", "3", only_6309=True, note=note)
_add("TFR", "1F", AddrMode.IMMEDIATE, OperandType.REG_PAIR, "6/4", "2")
_masked_memory("TIM", ("0B", "6B", "7B"))

for name, opcode, cycles, size, only in (
    ("TSTA", "4D", "2/1", "1", False), ("TSTB", "5D", "2/1", "1", False),
    ("TSTD", "04D", "3/2", "2", True), ("TSTE", "14D", "3/2", "2", True),
    ("TSTF", "15D", "3/2", "2", True), ("TSTW", "05D", "3/2", "2", True),
):
    _inherent(name, opcode, cycles, size, only_6309=only)
for mode, code, cyc, size in ((AddrMode.DIRECT, "0D", "6/4", "2"), (AddrMode.INDEXED, "6D", "6/5+", "2+"), (AddrMode.EXTENDED, "7D", "7/5", "3")):
    _add("TST", code, mode, {AddrMode.DIRECT: OperandType.DIRECT, AddrMode.INDEXED: OperandType.INDEXED, AddrMode.EXTENDED: OperandType.EXTENDED}[mode], cyc, size)


# Stable public table and lookup indexes.
INSTRUCTIONS: tuple[Instruction, ...] = tuple(_instructions)


def _indexed(
    form: str,
    pattern: str,
    indirect: bool,
    cycles: str,
    extra_bytes: int,
    *,
    only_6309: bool = False,
    description: str = "",
) -> IndexedForm:
    emulation, native = _split_cycles(cycles)
    return IndexedForm(
        assembler_form=form,
        postbyte_pattern=pattern,
        indirect=indirect,
        cycles_6809=None if only_6309 else emulation,
        cycles_6309_emulation=emulation,
        cycles_6309_native=native,
        extra_bytes=extra_bytes,
        only_6309=only_6309,
        description=description,
    )


# Additions to an instruction's base indexed cycle/byte count (reference p.150).
INDEXED_FORMS: tuple[IndexedForm, ...] = (
    _indexed(",R", "1RR00100", False, "0", 0, description="no offset"),
    _indexed("n,R", "0RRnnnnn", False, "1", 0, description="5-bit offset -16..15"),
    _indexed("n,R", "1RR01000", False, "1", 1, description="8-bit offset -128..127"),
    _indexed("n,R", "1RR01001", False, "4/3", 2, description="16-bit offset -32768..32767"),
    _indexed("[,R]", "1RR10100", True, "3", 0, description="no offset"),
    _indexed("[n,R]", "1RR11000", True, "4", 1, description="8-bit offset -128..127"),
    _indexed("[n,R]", "1RR11001", True, "7/6", 2, description="16-bit offset -32768..32767"),

    _indexed(",W", "10001111", False, "0", 0, only_6309=True, description="no offset"),
    _indexed("n,W", "10101111", False, "2", 2, only_6309=True, description="16-bit offset"),
    _indexed("[,W]", "10010000", True, "3", 0, only_6309=True, description="no offset"),
    _indexed("[n,W]", "10110000", True, "5", 2, only_6309=True, description="16-bit offset"),

    _indexed("A,R", "1RR00110", False, "1", 0, description="A accumulator offset"),
    _indexed("B,R", "1RR00101", False, "1", 0, description="B accumulator offset"),
    _indexed("D,R", "1RR01011", False, "4/2", 0, description="D accumulator offset"),
    _indexed("[A,R]", "1RR10110", True, "4", 0, description="A accumulator offset"),
    _indexed("[B,R]", "1RR10101", True, "4", 0, description="B accumulator offset"),
    _indexed("[D,R]", "1RR11011", True, "7/5", 0, description="D accumulator offset"),
    _indexed("E,R", "1RR00111", False, "1", 0, only_6309=True, description="E accumulator offset"),
    _indexed("F,R", "1RR01010", False, "1", 0, only_6309=True, description="F accumulator offset"),
    _indexed("W,R", "1RR01110", False, "1", 0, only_6309=True, description="W accumulator offset"),
    _indexed("[E,R]", "1RR10111", True, "4", 0, only_6309=True, description="E accumulator offset"),
    _indexed("[F,R]", "1RR11010", True, "4", 0, only_6309=True, description="F accumulator offset"),
    _indexed("[W,R]", "1RR11110", True, "4", 0, only_6309=True, description="W accumulator offset"),

    _indexed(",R+", "1RR00000", False, "2/1", 0, description="post-increment by 1"),
    _indexed(",R++", "1RR00001", False, "3/2", 0, description="post-increment by 2"),
    _indexed(",-R", "1RR00010", False, "2/1", 0, description="pre-decrement by 1"),
    _indexed(",--R", "1RR00011", False, "3/2", 0, description="pre-decrement by 2"),
    _indexed("[,R++]", "1RR10001", True, "6/5", 0, description="post-increment by 2"),
    _indexed("[,--R]", "1RR10011", True, "6/5", 0, description="pre-decrement by 2"),
    _indexed(",W++", "11001111", False, "1", 0, only_6309=True, description="post-increment by 2"),
    _indexed(",--W", "11101111", False, "1", 0, only_6309=True, description="pre-decrement by 2"),
    _indexed("[,W++]", "11010000", True, "4", 0, only_6309=True, description="post-increment by 2"),
    _indexed("[,--W]", "11110000", True, "4", 0, only_6309=True, description="pre-decrement by 2"),

    _indexed("n,PCR", "1XX01100", False, "1", 1, description="8-bit PC-relative offset"),
    _indexed("n,PCR", "1XX01101", False, "5/3", 2, description="16-bit PC-relative offset"),
    _indexed("[n,PCR]", "1XX11100", True, "4", 1, description="8-bit PC-relative offset"),
    _indexed("[n,PCR]", "1XX11101", True, "8/6", 2, description="16-bit PC-relative offset"),
    _indexed("[n]", "10011111", True, "5/4", 2, description="extended indirect address"),
)


# Postbyte codes useful to an assembler/disassembler.
INTER_REGISTER_CODES: Mapping[str, int] = MappingProxyType({
    "D": 0x0, "X": 0x1, "Y": 0x2, "U": 0x3,
    "S": 0x4, "PC": 0x5, "W": 0x6, "V": 0x7,
    "A": 0x8, "B": 0x9, "CC": 0xA, "DP": 0xB,
    "0": 0xC, "E": 0xE, "F": 0xF,
})
INTER_REGISTERS_6309_ONLY = frozenset({"W", "V", "0", "E", "F"})
BIT_TARGET_CODES: Mapping[str, int] = MappingProxyType({"CC": 0b00, "A": 0b01, "B": 0b10})


def _build_indexes(items: Iterable[Instruction]) -> tuple[
    Mapping[str, tuple[Instruction, ...]], Mapping[bytes, tuple[Instruction, ...]]
]:
    by_mnemonic: dict[str, list[Instruction]] = defaultdict(list)
    by_opcode: dict[bytes, list[Instruction]] = defaultdict(list)
    for item in items:
        by_mnemonic[item.mnemonic].append(item)
        for alias in item.aliases:
            by_mnemonic[alias].append(item)
        by_opcode[item.opcode].append(item)
    return (
        MappingProxyType({key: tuple(value) for key, value in by_mnemonic.items()}),
        MappingProxyType({key: tuple(value) for key, value in by_opcode.items()}),
    )


BY_MNEMONIC, BY_OPCODE = _build_indexes(INSTRUCTIONS)


def validate() -> None:
    """Raise ``AssertionError`` if the table contains an internal inconsistency."""
    seen: set[tuple[bytes, AddrMode]] = set()
    for item in INSTRUCTIONS:
        key = (item.opcode, item.mode)
        assert key not in seen, f"Duplicate opcode/mode: {item}"
        seen.add(key)
        assert item.only_6309 == (item.cycles_6809 is None), item
        assert item.minimum_byte_count >= len(item.opcode), item
        assert item.mnemonic == item.mnemonic.upper(), item
        assert all(alias == alias.upper() for alias in item.aliases), item


validate()


if __name__ == "__main__":
    n_6809 = sum(not item.only_6309 for item in INSTRUCTIONS)
    n_6309_only = sum(item.only_6309 for item in INSTRUCTIONS)
    print(f"{len(INSTRUCTIONS)} opcode/addressing forms: {n_6809} 6809 forms, "
          f"{n_6309_only} additional 6309-only forms")
    print(f"{len(BY_MNEMONIC)} accepted mnemonics/aliases, {len(BY_OPCODE)} opcodes")
