#!/usr/bin/env python3
"""Small, dependency-free two-pass Motorola 6809/Hitachi 6309 assembler.

The instruction encodings deliberately come from :mod:`ins6809`; this module
only parses source, selects an addressing form and emits bytes.
"""

from __future__ import annotations

import argparse
import ast
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from ins6809 import (
    AddrMode, BIT_TARGET_CODES, BY_MNEMONIC, INTER_REGISTER_CODES,
    INTER_REGISTERS_6309_ONLY, Instruction, OperandType,
)


class AssemblyError(Exception):
    """An error with source location."""


@dataclass
class Statement:
    line: int
    label: str | None
    operation: str | None
    operand: str
    source: str


@dataclass
class AssemblyResult:
    memory: bytearray
    used: set[int]
    symbols: dict[str, int]

    @property
    def range(self) -> tuple[int, int] | None:
        return (min(self.used), max(self.used) + 1) if self.used else None

    def binary(self, full: bool = True, fill: int = 0) -> bytes:
        if full:
            return bytes(self.memory)
        span = self.range
        return b"" if span is None else bytes(self.memory[slice(*span)])

    def intel_hex(self, record_size: int = 16) -> str:
        lines: list[str] = []
        addresses = sorted(self.used)
        pos = 0
        while pos < len(addresses):
            start = addresses[pos]
            data = bytearray([self.memory[start]])
            pos += 1
            while (pos < len(addresses) and addresses[pos] == start + len(data)
                   and len(data) < record_size):
                data.append(self.memory[addresses[pos]])
                pos += 1
            record = bytes((len(data), start >> 8, start & 255, 0)) + data
            checksum = (-sum(record)) & 255
            lines.append(":" + (record + bytes([checksum])).hex().upper())
        lines.append(":00000001FF")
        return "\n".join(lines) + "\n"


_NAME = re.compile(r"^[A-Za-z_.$][\w.$]*$")


def _strip_comment(line: str) -> str:
    quote = None
    for i, char in enumerate(line):
        if char in "\"'" and (i == 0 or line[i - 1] != "\\"):
            quote = None if quote == char else (char if quote is None else quote)
        elif char == ";" and quote is None:
            return line[:i]
    return line


def _split_csv(text: str) -> list[str]:
    result, part, quote, depth = [], [], None, 0
    for char in text:
        if char in "\"'" and (not part or part[-1] != "\\"):
            quote = None if quote == char else (char if quote is None else quote)
        if quote is None:
            depth += char in "(["
            depth -= char in ")]"
        if char == "," and quote is None and depth == 0:
            result.append("".join(part).strip()); part = []
        else:
            part.append(char)
    result.append("".join(part).strip())
    return result


def parse(source: str) -> list[Statement]:
    statements = []
    directives = {"ORG", "EQU", "SET", "FCB", "DB", "BYTE", "FDB", "DW",
                  "WORD", "RMB", "DS", "FILL", "CPU", "END"}
    for number, raw in enumerate(source.splitlines(), 1):
        text = _strip_comment(raw).strip()
        if not text:
            continue
        label = None
        match = re.match(r"^([A-Za-z_.$][\w.$]*):\s*(.*)$", text)
        if match:
            label, text = match.group(1).upper(), match.group(2).strip()
        if not text:
            statements.append(Statement(number, label, None, "", raw)); continue
        fields = text.split(None, 2)
        # Traditional assembler syntax permits "label EQU expression".
        if label is None and len(fields) >= 2 and fields[1].upper() in directives:
            label, text = fields[0].upper(), text[len(fields[0]):].lstrip()
        operation, _, operand = text.partition(" ")
        operation = operation.upper()
        if not _NAME.match(operation):
            raise AssemblyError(f"line {number}: invalid operation {operation!r}")
        statements.append(Statement(number, label, operation, operand.strip(), raw))
    return statements


class _Unknown(Exception): pass


def _expression(text: str, symbols: dict[str, int], pc: int, unknown_ok: bool = False) -> int:
    text = text.strip()
    # In Motorola syntax a leading '*' denotes the location counter; other
    # asterisks remain multiplication operators.
    text = re.sub(r"^\s*\*", "__PC__", text)
    def character(match: re.Match[str]) -> str:
        try:
            value = ast.literal_eval(match.group(0))
        except (SyntaxError, ValueError) as exc:
            raise AssemblyError(f"invalid character constant {match.group(0)!r}") from exc
        if len(value) != 1: raise AssemblyError("character constant must contain one character")
        return str(ord(value))
    text = re.sub(r"'(?:\\.|[^'\\])'", character, text)
    text = re.sub(r"\$([0-9A-Fa-f]+)", r"0x\1", text)
    text = re.sub(r"(?<![\w])%([01]+)", r"0b\1", text)
    # Names are case-insensitive, including names containing '.' and '$'.
    names: dict[str, int] = {"__PC__": pc}
    def replace_name(match: re.Match[str]) -> str:
        name = match.group(0)
        if name in names:
            return name
        if name in {"x", "X"} and match.start() and text[match.start()-1] == "0":
            return name
        key = name.upper()
        safe = f"__S{len(names)}"
        if key not in symbols:
            if unknown_ok: raise _Unknown(key)
            raise AssemblyError(f"unknown symbol {name!r}")
        names[safe] = symbols[key]
        return safe
    converted = re.sub(r"(?<![\w])[A-Za-z_.$][\w.$]*", replace_name, text)
    try:
        tree = ast.parse(converted, mode="eval")
    except SyntaxError as exc:
        raise AssemblyError(f"invalid expression {text!r}") from exc
    binary = {ast.Add: int.__add__, ast.Sub: int.__sub__, ast.Mult: int.__mul__,
              ast.FloorDiv: int.__floordiv__, ast.Mod: int.__mod__, ast.LShift: int.__lshift__,
              ast.RShift: int.__rshift__, ast.BitOr: int.__or__, ast.BitAnd: int.__and__, ast.BitXor: int.__xor__}
    unary = {ast.UAdd: lambda x: x, ast.USub: lambda x: -x, ast.Invert: int.__invert__}
    def visit(node: ast.AST) -> int:
        if isinstance(node, ast.Expression): return visit(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, str)):
            if isinstance(node.value, str):
                if len(node.value) != 1: raise AssemblyError("character constant must contain one character")
                return ord(node.value)
            return node.value
        if isinstance(node, ast.Name) and node.id in names: return names[node.id]
        if isinstance(node, ast.BinOp) and type(node.op) in binary: return binary[type(node.op)](visit(node.left), visit(node.right))
        if isinstance(node, ast.UnaryOp) and type(node.op) in unary: return unary[type(node.op)](visit(node.operand))
        raise AssemblyError(f"unsupported expression {text!r}")
    return visit(tree)


def _be(value: int, size: int) -> bytes:
    limit = 1 << (8 * size)
    if not -(limit // 2) <= value < limit:
        raise AssemblyError(f"value {value} does not fit in {size} byte(s)")
    return (value & (limit - 1)).to_bytes(size, "big")


class Assembler:
    def __init__(self, cpu: str = "6309", fill: int = 0):
        if cpu not in {"6809", "6309"}: raise ValueError("cpu must be 6809 or 6309")
        self.cpu, self.fill = cpu, fill & 255

    def assemble(self, source: str) -> AssemblyResult:
        statements = parse(source)
        symbols: dict[str, int] = {}
        # Repeat sizing because a forward reference can become direct or a short indexed offset.
        previous = None
        for _ in range(8):
            pc = 0
            layout: list[tuple[int, int]] = []
            for stmt in statements:
                try:
                    if stmt.label and stmt.operation not in {"EQU", "SET"}: symbols[stmt.label] = pc
                    data, new_pc = self._statement(stmt, pc, symbols, sizing=True)
                    layout.append((pc, len(data)))
                    pc = new_pc if new_pc is not None else pc + len(data)
                    if stmt.operation in {"EQU", "SET"}:
                        if not stmt.label: raise AssemblyError("EQU/SET requires a label")
                        symbols[stmt.label] = _expression(stmt.operand, symbols, pc, True)
                except _Unknown:
                    # Conservative sizes for unresolved expressions are returned by encoders;
                    # EQU itself must wait for the following iteration.
                    layout.append((pc, 0))
            state = (tuple(layout), tuple(sorted(symbols.items())))
            if state == previous: break
            previous = state
        memory = bytearray([self.fill]) * 65536
        used: set[int] = set(); pc = 0
        for stmt in statements:
            try:
                data, new_pc = self._statement(stmt, pc, symbols, sizing=False)
            except AssemblyError as exc:
                raise AssemblyError(f"line {stmt.line}: {exc}\n    {stmt.source}") from exc
            if new_pc is not None: pc = new_pc; continue
            if pc + len(data) > 0x10000: raise AssemblyError(f"line {stmt.line}: output exceeds 64K")
            for offset, byte in enumerate(data):
                address = pc + offset
                if address in used: raise AssemblyError(f"line {stmt.line}: address ${address:04X} written twice")
                memory[address] = byte; used.add(address)
            pc += len(data)
        return AssemblyResult(memory, used, symbols)

    def _statement(self, stmt: Statement, pc: int, symbols: dict[str, int], sizing: bool) -> tuple[bytes, int | None]:
        op = stmt.operation
        if op is None or op in {"EQU", "SET", "END", "CPU"}: return b"", None
        if op == "ORG": return b"", _expression(stmt.operand, symbols, pc, sizing)
        if op in {"FCB", "DB", "BYTE"}:
            out = bytearray()
            for item in _split_csv(stmt.operand):
                if len(item) >= 2 and item[0] == item[-1] == '"': out.extend(ast.literal_eval(item).encode("latin-1"))
                else: out.extend(_be(_expression(item, symbols, pc + len(out), sizing), 1))
            return bytes(out), None
        if op in {"FDB", "DW", "WORD"}:
            return b"".join(_be(_expression(x, symbols, pc, sizing), 2) for x in _split_csv(stmt.operand)), None
        if op in {"RMB", "DS"}:
            count = _expression(stmt.operand, symbols, pc, sizing)
            return bytes([self.fill]) * count, None
        if op == "FILL":
            value, count = _split_csv(stmt.operand)
            return bytes([_expression(value, symbols, pc, sizing) & 255]) * _expression(count, symbols, pc, sizing), None
        return self._instruction(op, stmt.operand, pc, symbols, sizing), None

    def _instruction(self, mnemonic: str, operand: str, pc: int, symbols: dict[str, int], sizing: bool) -> bytes:
        forms = [x for x in BY_MNEMONIC.get(mnemonic, ()) if self.cpu == "6309" or not x.only_6309]
        if not forms: raise AssemblyError(f"unknown or unavailable mnemonic {mnemonic!r}")
        if not operand:
            form = next((x for x in forms if x.operand is OperandType.NONE), None)
            if not form: raise AssemblyError("operand required")
            return form.opcode
        if operand.startswith("#"):
            candidates = [x for x in forms if x.mode is AddrMode.IMMEDIATE and x.operand in {OperandType.IMM8, OperandType.IMM16, OperandType.IMM32}]
            if not candidates: raise AssemblyError("immediate addressing is not supported")
            form = candidates[0]; size = {OperandType.IMM8: 1, OperandType.IMM16: 2, OperandType.IMM32: 4}[form.operand]
            return form.opcode + _be(_expression(operand[1:], symbols, pc, sizing), size)
        for typ in (OperandType.REG_PAIR, OperandType.REG_LIST, OperandType.TFM_REG_PAIR, OperandType.BIT_TRANSFER):
            candidate = next((x for x in forms if x.operand is typ), None)
            if candidate:
                if typ is OperandType.TFM_REG_PAIR:
                    pair = [x.strip().upper() for x in _split_csv(operand)]
                    shapes = (("+", "+"), ("-", "-"), ("+", ""), ("", "+"))
                    if len(pair) != 2: raise AssemblyError("TFM expects two registers")
                    suffix = tuple("+" if x.endswith("+") else "-" if x.endswith("-") else "" for x in pair)
                    try: candidate = [x for x in forms if x.operand is typ][shapes.index(suffix)]
                    except ValueError as exc: raise AssemblyError("invalid TFM increment/decrement combination") from exc
                return candidate.opcode + self._special(candidate, operand, symbols, pc, sizing)
        relative = next((x for x in forms if x.mode is AddrMode.RELATIVE), None)
        if relative:
            size = 1 if relative.operand is OperandType.REL8 else 2
            target = _expression(operand, symbols, pc, sizing)
            return relative.opcode + _be(target - (pc + len(relative.opcode) + size), size)
        masked = next((x for x in forms if x.operand in {OperandType.IMM8_DIRECT, OperandType.IMM8_INDEXED, OperandType.IMM8_EXTENDED}), None)
        if masked:
            mask, address = _split_csv(operand)
            if mask.startswith("#"): mask = mask[1:]
            prefix = _be(_expression(mask, symbols, pc, sizing), 1)
            normal = [x for x in forms if x.operand in {OperandType.IMM8_DIRECT, OperandType.IMM8_INDEXED, OperandType.IMM8_EXTENDED}]
            form, encoded = self._memory(normal, address, pc + 1, symbols, sizing)
            return form.opcode + prefix + encoded
        form, encoded = self._memory(forms, operand, pc, symbols, sizing)
        return form.opcode + encoded

    def _memory(self, forms: Iterable[Instruction], operand: str, pc: int, symbols: dict[str, int], sizing: bool) -> tuple[Instruction, bytes]:
        forms = list(forms)
        if "," in operand or operand.strip().startswith("["):
            form = next((x for x in forms if x.mode is AddrMode.INDEXED), None)
            if not form: raise AssemblyError("indexed addressing is not supported")
            return form, self._indexed(operand, pc + len(form.opcode), symbols, sizing)
        force = operand[:1] if operand[:1] in "<>" else ""
        expr = operand[1:] if force else operand
        try: value = _expression(expr, symbols, pc, sizing)
        except _Unknown:
            value = 0x100; force = ">"
        direct = next((x for x in forms if x.mode is AddrMode.DIRECT), None)
        extended = next((x for x in forms if x.mode is AddrMode.EXTENDED), None)
        if force != ">" and direct and (force == "<" or 0 <= value <= 255): return direct, _be(value, 1)
        if extended: return extended, _be(value, 2)
        if direct: return direct, _be(value, 1)
        raise AssemblyError("no suitable addressing mode")

    def _indexed(self, text: str, pc_after_opcode: int, symbols: dict[str, int], sizing: bool) -> bytes:
        indirect = text.strip().startswith("[")
        inner = text.strip()[1:-1].strip() if indirect else text.strip()
        if "," not in inner:  # [extended]
            if not indirect: raise AssemblyError("invalid indexed operand")
            return b"\x9f" + _be(_expression(inner, symbols, pc_after_opcode, sizing), 2)
        left, reg = (x.strip().upper() for x in inner.rsplit(",", 1))
        regs = {"X": 0, "Y": 1, "U": 2, "S": 3}
        if reg == "PCR":
            target = _expression(left, symbols, pc_after_opcode, sizing)
            delta8 = target - (pc_after_opcode + 2)
            if -128 <= delta8 <= 127:
                return bytes([0x9C if indirect else 0x8C, delta8 & 255])
            delta16 = target - (pc_after_opcode + 3)
            return bytes([0x9D if indirect else 0x8D]) + _be(delta16, 2)
        match = re.fullmatch(r"(--)?(X|Y|U|S|W)(\+\+?)?", reg)
        if not match: raise AssemblyError(f"invalid index register {reg!r}")
        pre, base, post = match.groups()
        if base == "W":
            if self.cpu != "6309": raise AssemblyError("W indexing requires 6309")
            codes = {(False, None, None): 0x8F, (True, None, None): 0x90,
                     (False, None, "++"): 0xCF, (True, None, "++"): 0xD0,
                     (False, "--", None): 0xEF, (True, "--", None): 0xF0}
            if left and not pre and not post:
                return bytes([0xB0 if indirect else 0xAF]) + _be(_expression(left, symbols, pc_after_opcode, sizing), 2)
            key = (indirect, pre, post)
            if left or key not in codes: raise AssemblyError("invalid W indexed form")
            return bytes([codes[key]])
        rr = regs[base] << 5
        if pre or post:
            codes = {(None, "+"): 0x80, (None, "++"): 0x81, ("--", None): 0x83}
            if indirect: codes = {(None, "++"): 0x91, ("--", None): 0x93}
            if left or (pre, post) not in codes: raise AssemblyError("invalid auto increment/decrement form")
            return bytes([codes[(pre, post)] | rr])
        acc_codes = {"A": 0x86, "B": 0x85, "D": 0x8B, "E": 0x87, "F": 0x8A, "W": 0x8E}
        if left in acc_codes:
            if left in "EFW" and self.cpu != "6309": raise AssemblyError(f"{left} offset requires 6309")
            return bytes([acc_codes[left] + (0x10 if indirect else 0) | rr])
        if not left: return bytes([(0x94 if indirect else 0x84) | rr])
        try: value = _expression(left, symbols, pc_after_opcode, sizing)
        except _Unknown: value = 0x8000
        if not indirect and -16 <= value <= 15: return bytes([rr | (value & 0x1f)])
        if -128 <= value <= 127: return bytes([(0x98 if indirect else 0x88) | rr, value & 255])
        return bytes([(0x99 if indirect else 0x89) | rr]) + _be(value, 2)

    def _special(self, form: Instruction, text: str, symbols: dict[str, int], pc: int, sizing: bool) -> bytes:
        parts = [x.strip().upper() for x in _split_csv(text)]
        if form.operand is OperandType.REG_PAIR:
            if len(parts) != 2 or any(x not in INTER_REGISTER_CODES for x in parts): raise AssemblyError("expected two registers")
            if self.cpu == "6809" and any(x in INTER_REGISTERS_6309_ONLY for x in parts): raise AssemblyError("register requires 6309")
            return bytes([(INTER_REGISTER_CODES[parts[0]] << 4) | INTER_REGISTER_CODES[parts[1]]])
        if form.operand is OperandType.REG_LIST:
            codes = {"CC": 1, "A": 2, "B": 4, "DP": 8, "X": 16, "Y": 32, "PC": 128,
                     ("U" if form.mnemonic.endswith("S") else "S"): 64}
            if any(x not in codes for x in parts): raise AssemblyError("invalid register in push/pull list")
            return bytes([sum(codes[x] for x in set(parts))])
        if form.operand is OperandType.TFM_REG_PAIR:
            if len(parts) != 2: raise AssemblyError("TFM expects two registers")
            base = {"D": 0, "X": 1, "Y": 2, "U": 3, "S": 4}
            clean = [x.rstrip("+-") for x in parts]
            if any(x not in base for x in clean): raise AssemblyError("invalid TFM register")
            return bytes([(base[clean[0]] << 4) | base[clean[1]]])
        # BAND/BOR/... syntax: register,source-bit,destination-bit,direct-address
        if len(parts) != 4 or parts[0] not in BIT_TARGET_CODES: raise AssemblyError("expected register,srcbit,dstbit,address")
        src, dst = (_expression(x, symbols, pc, sizing) for x in parts[1:3])
        if not 0 <= src <= 7 or not 0 <= dst <= 7: raise AssemblyError("bit number must be 0..7")
        return bytes([(BIT_TARGET_CODES[parts[0]] << 6) | (src << 3) | dst]) + _be(_expression(parts[3], symbols, pc, sizing), 1)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Motorola 6809 / Hitachi 6309 assembler")
    parser.add_argument("source", type=Path)
    parser.add_argument("-o", "--output", type=Path, required=True)
    parser.add_argument("--cpu", choices=("6809", "6309"), default="6309")
    parser.add_argument("--format", choices=("bin", "hex"), default="bin")
    parser.add_argument("--trim", action="store_true", help="binary: write only the used address span")
    parser.add_argument("--fill", type=lambda x: int(x, 0), default=0)
    args = parser.parse_args(argv)
    try:
        result = Assembler(args.cpu, args.fill).assemble(args.source.read_text(encoding="utf-8"))
        if args.format == "hex": args.output.write_text(result.intel_hex(), encoding="ascii")
        else: args.output.write_bytes(result.binary(full=not args.trim))
    except (AssemblyError, OSError) as exc:
        parser.exit(1, f"error: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
