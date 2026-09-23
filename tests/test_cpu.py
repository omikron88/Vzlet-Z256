import unittest

from mc6809 import HD6309, MC6809, IllegalInstruction


class Memory:
    def __init__(self):
        self.data = bytearray(65536)
        self.log = []
    def read(self, address):
        self.log.append(("r", address))
        return self.data[address]
    def write(self, address, value):
        self.log.append(("w", address, value))
        self.data[address] = value
    def word(self, address, value):
        self.data[address] = value >> 8
        self.data[(address + 1) & 0xffff] = value & 255


class CPUTest(unittest.TestCase):
    def setUp(self):
        self.mem = Memory()
        self.mem.word(0xfffe, 0x1000)
        self.cpu = MC6809(self.mem.read, self.mem.write)
        self.cpu.reset()

    def load(self, *values):
        self.mem.data[self.cpu.r.pc:self.cpu.r.pc + len(values)] = bytes(values)

    def test_callbacks_reset_and_arithmetic(self):
        self.load(0x86, 0x7f, 0x8b, 0x01, 0x97, 0x42)
        self.cpu.r.dp = 0x20
        self.assertEqual(self.cpu.step(), 2)
        self.assertEqual(self.cpu.step(), 2)
        self.assertEqual(self.cpu.r.a, 0x80)
        self.assertTrue(self.cpu.r.cc & 0x02)  # overflow
        self.assertEqual(self.cpu.step(), 4)
        self.assertEqual(self.mem.data[0x2042], 0x80)

    def test_big_endian_word_and_extended_load(self):
        self.mem.word(0x3456, 0xabcd)
        self.load(0xbe, 0x34, 0x56)  # LDX $3456
        self.cpu.step()
        self.assertEqual(self.cpu.r.x, 0xabcd)

    def test_indexed_postincrement(self):
        self.cpu.r.x = 0x3000
        self.mem.data[0x3000] = 0x55
        self.load(0xa6, 0x80)  # LDA ,X+
        self.cpu.step()
        self.assertEqual((self.cpu.r.a, self.cpu.r.x), (0x55, 0x3001))

    def test_subroutine_and_return(self):
        self.cpu.r.s = 0x4000
        self.load(0xbd, 0x11, 0x00)
        self.mem.data[0x1100] = 0x39
        self.cpu.step()
        self.assertEqual(self.cpu.r.pc, 0x1100)
        self.cpu.step()
        self.assertEqual(self.cpu.r.pc, 0x1003)

    def test_irq_frame_and_rti(self):
        self.cpu.r.s = 0x5000
        self.cpu.r.cc &= ~0x10
        self.mem.word(0xfff8, 0x2200)
        self.mem.data[0x2200] = 0x3b
        self.cpu.irq()
        self.assertEqual(self.cpu.step(), 19)
        self.assertEqual(self.cpu.r.pc, 0x2200)
        self.cpu.irq(False)
        self.cpu.step()
        self.assertEqual(self.cpu.r.pc, 0x1000)
        self.assertEqual(self.cpu.r.s, 0x5000)

    def test_illegal_opcode_has_context(self):
        self.load(0x01)
        with self.assertRaises(IllegalInstruction) as error:
            self.cpu.step()
        self.assertEqual(error.exception.pc, 0x1000)

    def test_6309_registers_and_lde(self):
        cpu = HD6309(self.mem.read, self.mem.write)
        cpu.reset()
        self.mem.data[0x1000:0x1003] = bytes([0x11, 0x86, 0x7e])
        cpu.step()
        self.assertEqual(cpu.r.e, 0x7e)
        cpu.r.q = 0x12345678
        self.assertEqual((cpu.r.d, cpu.r.w), (0x1234, 0x5678))


if __name__ == "__main__":
    unittest.main()
