import tempfile
import unittest
from pathlib import Path

from assembler import Assembler, AssemblyError, main


class AssemblerTests(unittest.TestCase):
    def test_two_pass_modes_labels_and_data(self):
        result = Assembler("6809", fill=0xFF).assemble("""
            org $1000
        start: lda #$42
            sta <$20
            leax message,pcr
            bra start
        message: db "OK", 0
            dw start
        """)
        self.assertEqual(result.memory[0x1000:0x100E], bytes.fromhex("86429720308C0220F74F4B001000"))
        self.assertEqual(result.symbols["MESSAGE"], 0x1009)
        self.assertEqual(result.range, (0x1000, 0x100E))

    def test_indexed_forms_and_registers(self):
        result = Assembler("6309").assemble("""
            org $200
            lda 5,x
            ldb [300,y]
            pshs a,b,x,pc
            tfr d,x
            tfm x+,y+
        """)
        self.assertEqual(result.memory[0x200:0x20D], bytes.fromhex("A605E6B9012C34961F01113812"))

    def test_direct_extended_and_6309_guard(self):
        result = Assembler("6809").assemble("lda $12\nlda >$12\n")
        self.assertEqual(result.memory[:5], bytes.fromhex("9612B60012"))
        with self.assertRaises(AssemblyError):
            Assembler("6809").assemble("ldw #1")

    def test_intel_hex_sparse_records(self):
        result = Assembler().assemble("org $1000\ndb 1,2,3\norg $2000\ndb $ff")
        self.assertEqual(result.intel_hex(), ":03100000010203E7\n:01200000FFE0\n:00000001FF\n")

    def test_expressions_and_location_counter(self):
        result = Assembler().assemble("org $10\ndb 'A', 2*3, *-$10")
        self.assertEqual(result.memory[0x10:0x13], b"A\x06\x02")

    def test_hi_lo_functions_are_case_insensitive_and_accept_expressions(self):
        result = Assembler().assemble("""
            address equ $ABCD
            db Hi(address), lo(address), HI(address + $100), Lo($123456)
            lda #hi(address)
            ldb #LO(address)
        """)
        self.assertEqual(result.memory[:8], bytes.fromhex("ABCDA C5686AB C6CD".replace(" ", "")))

    def test_hi_lo_require_one_argument(self):
        with self.assertRaisesRegex(AssemblyError, r"HI\(\) expects exactly one argument"):
            Assembler().assemble("db hi(1, 2)")

    def test_cli_binary_and_hex(self):
        with tempfile.TemporaryDirectory() as directory:
            src, out = Path(directory) / "a.asm", Path(directory) / "a.hex"
            src.write_text("org $fffe\ndw $1234\n", encoding="utf-8")
            self.assertEqual(main([str(src), "-o", str(out), "--format", "hex"]), 0)
            self.assertIn(":02FFFE001234BB", out.read_text(encoding="ascii"))


if __name__ == "__main__":
    unittest.main()
