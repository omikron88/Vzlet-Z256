# mc6809

A small, dependency-free Python emulation library for the Motorola MC6809 and
Hitachi HD6309. The CPU has no built-in RAM: **every memory access is performed
through host callbacks**. This permits bank switching, memory-mapped devices,
breakpoints, and bus tracing without modifying the CPU.

## Quick start

```python
from mc6809 import MC6809

memory = bytearray(65536)
def Read(address: int) -> int:
    return memory[address]
def Write(address: int, value: int) -> None:
    memory[address] = value

memory[0xfffe:0x10000] = (0x1000).to_bytes(2, "big")
memory[0x1000:0x1005] = bytes([0x86, 0x2a, 0x8b, 0x10, 0x12])

cpu = MC6809(Read, Write)
cpu.reset()
cpu.step()                  # LDA #$2a
cpu.step()                  # ADDA #$10
assert cpu.r.a == 0x3a
```

Use `HD6309(Read, Write)` for the enhanced variant. Registers are intentionally
public in `cpu.r`; 16/32-bit `d`, `w`, and `q` properties keep their component
registers synchronized. `step()` returns instruction cycles, `run(n)` executes
whole instructions to a cycle budget, and `cpu.cycles` contains the total.

## Host interface

* `Read(address) -> int` is called once per byte read. Addresses are masked to
  16 bits and results to 8 bits.
* `Write(address, value)` is called once per byte write, with both values masked.
* `irq(bool)` and `firq(bool)` set level-sensitive interrupt lines; `nmi()`
  latches an edge. Interrupt vectors and reset vector are fetched through Read.
* `IllegalInstruction` reports the instruction address, prefix page, and opcode.

The implementation covers the complete regular 6809 ALU/addressing matrix,
branches, stack and control instructions, all indexed postbyte forms, and
interrupt entry/return. The 6309 class exposes E/F, W, V, Q and MD and implements
register transfer plus common E/F and MD instructions. Remaining specialised
6309 operations deliberately raise `IllegalInstruction` rather than silently
behaving as a 6809 instruction, providing a safe extension point.

## Accuracy and integration

Cycle values model documented instruction-level timing. Callback accesses are
in architectural byte order, but dummy/internal bus cycles are not emitted.
Consequently this core is intended for machine emulation and tooling rather
than electrical, cycle-pin-exact simulation.

Install for development and run tests:

```sh
python -m pip install -e .
python -m unittest discover -s tests -v
```
