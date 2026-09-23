"""Motorola MC6809 and Hitachi HD6309 emulation core.

The core deliberately owns no memory.  Every bus cycle goes through the two
callbacks supplied by the host, which makes memory mapping and tracing simple.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Literal

Read = Callable[[int], int]
Write = Callable[[int, int], None]

# Condition-code bits
E, F, H, I, N, Z, V, C = (0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01)


class IllegalInstruction(RuntimeError):
    """Raised when the CPU encounters an unsupported/illegal opcode."""

    def __init__(self, pc: int, opcode: int, page: int = 0):
        self.pc, self.opcode, self.page = pc, opcode, page
        prefix = f"{page:02X} " if page else ""
        super().__init__(f"illegal instruction {prefix}{opcode:02X} at {pc:04X}")


@dataclass
class Registers:
    """Public, mutable programmer-visible CPU state."""

    a: int = 0
    b: int = 0
    x: int = 0
    y: int = 0
    u: int = 0
    s: int = 0
    pc: int = 0
    dp: int = 0
    cc: int = I | F
    # HD6309 registers (harmless and accessible on an MC6809 instance).
    e: int = 0
    f: int = 0
    v: int = 0
    md: int = 0

    @property
    def d(self) -> int:
        return (self.a << 8) | self.b

    @d.setter
    def d(self, value: int) -> None:
        self.a, self.b = (value >> 8) & 0xFF, value & 0xFF

    @property
    def w(self) -> int:
        return (self.e << 8) | self.f

    @w.setter
    def w(self, value: int) -> None:
        self.e, self.f = (value >> 8) & 0xFF, value & 0xFF

    @property
    def q(self) -> int:
        return (self.d << 16) | self.w

    @q.setter
    def q(self, value: int) -> None:
        self.d, self.w = (value >> 16) & 0xFFFF, value & 0xFFFF


class CPU:
    """Cycle-counted callback-driven 6809 family core.

    ``read(address)`` must return a byte; ``write(address, byte)`` receives
    already masked values.  :meth:`step` executes one instruction and returns
    its cycle count. Interrupt input methods latch a line until serviced.
    """

    def __init__(self, read: Read, write: Write, *, variant: Literal["6809", "6309"] = "6809"):
        if not callable(read) or not callable(write):
            raise TypeError("read and write must be callable")
        if variant not in ("6809", "6309"):
            raise ValueError("variant must be '6809' or '6309'")
        self.read, self.write, self.variant = read, write, variant
        self.r = Registers()
        self.cycles = 0
        self.waiting = False
        self.syncing = False
        self._irq = self._firq = self._nmi = False

    # ---- bus and lifecycle -------------------------------------------------
    def rb(self, address: int) -> int:
        return self.read(address & 0xFFFF) & 0xFF

    def wb(self, address: int, value: int) -> None:
        self.write(address & 0xFFFF, value & 0xFF)

    def rw(self, address: int) -> int:
        return (self.rb(address) << 8) | self.rb(address + 1)

    def ww(self, address: int, value: int) -> None:
        self.wb(address, value >> 8); self.wb(address + 1, value)

    def reset(self) -> None:
        self.r = Registers()
        self.r.pc = self.rw(0xFFFE)
        self.waiting = self.syncing = False
        self._irq = self._firq = self._nmi = False
        self.cycles = 0

    def irq(self, active: bool = True) -> None: self._irq = active
    def firq(self, active: bool = True) -> None: self._firq = active
    def nmi(self) -> None: self._nmi = True

    def run(self, cycle_budget: int) -> int:
        """Run whole instructions until at least *cycle_budget* cycles elapse."""
        spent = 0
        while spent < cycle_budget:
            spent += self.step()
        return spent

    # ---- fetch, stack, flags ----------------------------------------------
    def _fetch8(self) -> int:
        v = self.rb(self.r.pc); self.r.pc = (self.r.pc + 1) & 0xFFFF; return v

    def _fetch16(self) -> int:
        return (self._fetch8() << 8) | self._fetch8()

    @staticmethod
    def _sx8(v: int) -> int: return v - 0x100 if v & 0x80 else v
    @staticmethod
    def _sx16(v: int) -> int: return v - 0x10000 if v & 0x8000 else v

    def _push8(self, stack: str, value: int) -> None:
        p = (getattr(self.r, stack) - 1) & 0xFFFF
        setattr(self.r, stack, p); self.wb(p, value)

    def _push16(self, stack: str, value: int) -> None:
        self._push8(stack, value & 0xFF); self._push8(stack, value >> 8)

    def _pull8(self, stack: str) -> int:
        p = getattr(self.r, stack); v = self.rb(p)
        setattr(self.r, stack, (p + 1) & 0xFFFF); return v

    def _pull16(self, stack: str) -> int:
        return (self._pull8(stack) << 8) | self._pull8(stack)

    def _nz(self, value: int, bits: int) -> int:
        mask, sign = (1 << bits) - 1, 1 << (bits - 1)
        self.r.cc &= ~(N | Z | V)
        value &= mask
        if value == 0: self.r.cc |= Z
        if value & sign: self.r.cc |= N
        return value

    def _logic(self, value: int, bits: int) -> int:
        self.r.cc &= ~V
        return self._nz(value, bits)

    def _add(self, left: int, right: int, bits: int, carry: int = 0) -> int:
        mask, sign = (1 << bits) - 1, 1 << (bits - 1)
        total = left + right + carry; result = total & mask
        self.r.cc &= ~(N | Z | V | C | (H if bits == 8 else 0))
        if result == 0: self.r.cc |= Z
        if result & sign: self.r.cc |= N
        if (~(left ^ right) & (left ^ result) & sign): self.r.cc |= V
        if total > mask: self.r.cc |= C
        if bits == 8 and ((left & 15) + (right & 15) + carry > 15): self.r.cc |= H
        return result

    def _sub(self, left: int, right: int, bits: int, carry: int = 0) -> int:
        mask, sign = (1 << bits) - 1, 1 << (bits - 1)
        result = (left - right - carry) & mask
        self.r.cc &= ~(N | Z | V | C)
        if result == 0: self.r.cc |= Z
        if result & sign: self.r.cc |= N
        if ((left ^ right) & (left ^ result) & sign): self.r.cc |= V
        if left < right + carry: self.r.cc |= C
        return result

    # ---- addressing -------------------------------------------------------
    def _ea(self, mode: int) -> tuple[int, int]:
        if mode == 0x0: return (self.r.dp << 8) | self._fetch8(), 0
        if mode == 0x2: return self._fetch16(), 1
        if mode != 0x1: raise AssertionError(mode)
        post = self._fetch8()
        regs = ("x", "y", "u", "s"); reg = regs[(post >> 5) & 3]
        base = getattr(self.r, reg)
        if not post & 0x80: return (base + self._sx8(post & 0x1F | (0xE0 if post & 0x10 else 0))) & 0xFFFF, 1
        indirect, sub = bool(post & 0x10), post & 0x0F; extra = 0
        if sub == 0: ea = base; setattr(self.r, reg, (base + 1) & 0xFFFF); extra = 2
        elif sub == 1: ea = base; setattr(self.r, reg, (base + 2) & 0xFFFF); extra = 3
        elif sub == 2: setattr(self.r, reg, (base - 1) & 0xFFFF); ea = getattr(self.r, reg); extra = 2
        elif sub == 3: setattr(self.r, reg, (base - 2) & 0xFFFF); ea = getattr(self.r, reg); extra = 3
        elif sub == 4: ea = base
        elif sub == 5: ea = (base + self._sx8(self.r.b)) & 0xFFFF; extra = 1
        elif sub == 6: ea = (base + self._sx8(self.r.a)) & 0xFFFF; extra = 1
        elif sub == 8: ea = (base + self._sx8(self._fetch8())) & 0xFFFF; extra = 1
        elif sub == 9: ea = (base + self._fetch16()) & 0xFFFF; extra = 4
        elif sub == 11: ea = (base + self._sx16(self.r.d)) & 0xFFFF; extra = 4
        elif sub == 12: ea = (self.r.pc + self._sx8(self._fetch8())) & 0xFFFF; extra = 1
        elif sub == 13: ea = (self.r.pc + self._sx16(self._fetch16())) & 0xFFFF; extra = 5
        elif sub == 15 and indirect: ea = self._fetch16(); extra = 2
        else: raise IllegalInstruction((self.r.pc - 2) & 0xFFFF, post)
        if indirect: ea = self.rw(ea); extra += 3
        return ea, extra

    def _operand(self, mode: int, bits: int) -> tuple[int, int]:
        if mode == 3: return (self._fetch8() if bits == 8 else self._fetch16()), 0
        ea, extra = self._ea(mode); return (self.rb(ea) if bits == 8 else self.rw(ea)), extra

    # ---- interrupts -------------------------------------------------------
    def _frame(self, whole: bool) -> None:
        if whole:
            self.r.cc |= E
            for v, wide in ((self.r.pc,1),(self.r.u,1),(self.r.y,1),(self.r.x,1),(self.r.dp,0),(self.r.b,0),(self.r.a,0),(self.r.cc,0)):
                (self._push16 if wide else self._push8)("s", v)
        else:
            self.r.cc &= ~E; self._push16("s", self.r.pc); self._push8("s", self.r.cc)

    def _interrupts(self) -> int:
        if self._nmi:
            self._nmi = False; self._frame(True); self.r.cc |= I | F; self.r.pc = self.rw(0xFFFC); return 19
        if self._firq and not self.r.cc & F:
            self._frame(False); self.r.cc |= I | F; self.r.pc = self.rw(0xFFF6); return 10
        if self._irq and not self.r.cc & I:
            self._frame(True); self.r.cc |= I; self.r.pc = self.rw(0xFFF8); return 19
        return 0

    # ---- instruction execution ------------------------------------------
    def step(self) -> int:
        intr = self._interrupts()
        if intr:
            self.waiting = self.syncing = False; self.cycles += intr; return intr
        if self.waiting or self.syncing:
            self.cycles += 1; return 1
        start = self.r.pc; op = self._fetch8(); page = 0
        if op in (0x10, 0x11): page, op = op, self._fetch8()
        n = self._execute(page, op, start)
        self.cycles += n
        return n

    def _execute(self, page: int, op: int, start: int) -> int:
        # branches, including prefixed long conditional branches
        conditions = [True, False, not (self.r.cc & (C|Z)), bool(self.r.cc & (C|Z)),
            not (self.r.cc & C), bool(self.r.cc & C), not (self.r.cc & Z), bool(self.r.cc & Z),
            not (self.r.cc & V), bool(self.r.cc & V), not (self.r.cc & N), bool(self.r.cc & N),
            not bool((self.r.cc & N) ^ ((self.r.cc & V) << 2)), bool((self.r.cc & N) ^ ((self.r.cc & V) << 2)),
            not (self.r.cc & Z) and not bool((self.r.cc & N) ^ ((self.r.cc & V) << 2)),
            bool(self.r.cc & Z) or bool((self.r.cc & N) ^ ((self.r.cc & V) << 2))]
        if page == 0 and 0x20 <= op <= 0x2F:
            d = self._sx8(self._fetch8()); self.r.pc = (self.r.pc + d) & 0xFFFF if conditions[op&15] else self.r.pc; return 3
        if page == 0x10 and 0x20 <= op <= 0x2F:
            d = self._sx16(self._fetch16()); self.r.pc = (self.r.pc + d) & 0xFFFF if conditions[op&15] else self.r.pc; return 5 if conditions[op&15] else 4

        if page == 0:
            fixed = {0x12:2, 0x13:4, 0x19:2, 0x1A:3, 0x1C:3, 0x1D:2, 0x39:5, 0x3B:6, 0x3C:20, 0x3D:11, 0x3F:19}
            if op == 0x12: return 2
            if op == 0x13: self.syncing=True; return 4
            if op == 0x19: # DAA
                adj = (0x60 if self.r.a > 0x99 or self.r.cc&C else 0) + (0x06 if (self.r.a&15)>9 or self.r.cc&H else 0)
                old=self.r.a; self.r.a=(old+adj)&255; self._nz(self.r.a,8); self.r.cc &= ~V
                if old+adj>255:self.r.cc|=C
                return 2
            if op == 0x1A: self.r.cc |= self._fetch8(); return 3
            if op == 0x1C: self.r.cc &= self._fetch8(); return 3
            if op == 0x1D: self.r.d = self.r.b | (0xFF00 if self.r.b&0x80 else 0); self._nz(self.r.d,16); return 2
            if op in (0x1E,0x1F): return self._exg_tfr(op == 0x1E)
            if op == 0x16: self.r.pc=(self.r.pc+self._sx16(self._fetch16()))&0xFFFF; return 5
            if op == 0x17: d=self._sx16(self._fetch16()); self._push16("s",self.r.pc); self.r.pc=(self.r.pc+d)&0xFFFF; return 9
            if op == 0x30 or op == 0x31 or op == 0x32 or op == 0x33:
                ea,x=self._ea(1); setattr(self.r,("x","y","s","u")[op-0x30],ea)
                if op<0x32: self._nz(ea,16)
                return 4+x
            if op in (0x34,0x35,0x36,0x37): return self._stack_op(op)
            if op == 0x39: self.r.pc=self._pull16("s"); return 5
            if op == 0x3A: self.r.x=(self.r.x+self.r.b)&0xFFFF; return 3
            if op == 0x3B:
                self.r.cc=self._pull8("s")
                if self.r.cc&E:
                    self.r.a=self._pull8("s");self.r.b=self._pull8("s");self.r.dp=self._pull8("s");self.r.x=self._pull16("s");self.r.y=self._pull16("s");self.r.u=self._pull16("s")
                    self.r.pc=self._pull16("s")
                    return 15
                self.r.pc=self._pull16("s")
                return 6
            if op == 0x3C: self.r.cc &= self._fetch8(); self._frame(True); self.waiting=True; return 20
            if op == 0x3D: self.r.d=self.r.a*self.r.b; self.r.cc=(self.r.cc&~(Z|C))|(Z if self.r.d==0 else 0)|(C if self.r.b&0x80 else 0); return 11
            if op == 0x3F: self._frame(True);self.r.cc|=I|F;self.r.pc=self.rw(0xFFFA);return 19
            if op == 0x8D: d=self._sx8(self._fetch8());self._push16("s",self.r.pc);self.r.pc=(self.r.pc+d)&0xFFFF;return 7
            if op in (0x6E,0x7E): ea,x=self._ea(1 if op==0x6E else 2);self.r.pc=ea;return 3+x
            if op in (0x9D,0xAD,0xBD): ea,x=self._ea(((op>>4)&3)-1);self._push16("s",self.r.pc);self.r.pc=ea;return 7+x
        if page in (0x10,0x11) and op == 0x3F:
            self._frame(True); self.r.cc |= I|F; self.r.pc=self.rw(0xFFF4 if page==0x10 else 0xFFF2); return 20

        # Unary accumulator/memory operations.
        if page == 0 and ((0x40 <= op <= 0x5F) or op < 0x10 or 0x60 <= op <= 0x7F):
            return self._unary(op, start)

        # Regular ALU matrix: immediate/direct/indexed/extended.
        if page == 0 and op >= 0x80:
            result = self._alu(op)
            if result is not None: return result
        # Page 10 adds D/Y/S comparisons and Y/S loads/stores.
        if page == 0x10:
            result = self._page10(op)
            if result is not None: return result
        if page == 0x11:
            result = self._page11(op)
            if result is not None: return result
        raise IllegalInstruction(start, op, page)

    def _unary(self, op: int, start: int) -> int:
        nib=op&15
        if 0x40<=op<=0x4F: val=self.r.a; target="a"; base=2; ea=None
        elif 0x50<=op<=0x5F: val=self.r.b; target="b"; base=2; ea=None
        else:
            mode=0 if op<0x10 else (1 if op<0x70 else 2); ea,x=self._ea(mode); val=self.rb(ea);target="";base=(6 if mode==0 else 6+x if mode==1 else 7)
        old=val
        if nib==0: val=self._sub(0,val,8)
        elif nib==3: val=(~val)&255;self._nz(val,8);self.r.cc=(self.r.cc&~(V|C))|C
        elif nib==4: self.r.cc=(self.r.cc&~(N|Z|C))|(C if val&1 else 0);val>>=1;self.r.cc|=Z if val==0 else 0
        elif nib==6:
            c=1 if self.r.cc&C else 0;self.r.cc&=~(N|Z|C);self.r.cc|=C if val&1 else 0;val=(val>>1)|(c<<7);self.r.cc|=(N if val&128 else 0)|(Z if val==0 else 0);self.r.cc=(self.r.cc&~V)| (V if bool(self.r.cc&N)^bool(self.r.cc&C) else 0)
        elif nib==7: self.r.cc&=~(N|Z|C);self.r.cc|=C if val&1 else 0;val=((val>>1)|(val&128));self.r.cc|=(N if val&128 else 0)|(Z if val==0 else 0)
        elif nib==8:
            self.r.cc&=~(N|Z|C);self.r.cc|=C if val&128 else 0;val=(val<<1)&255;self.r.cc|=(N if val&128 else 0)|(Z if val==0 else 0);self.r.cc=(self.r.cc&~V)|(V if bool(self.r.cc&N)^bool(self.r.cc&C) else 0)
        elif nib==9: val=self._add(val,0,8,1 if self.r.cc&C else 0) # ROL via add with carry
        elif nib==10: val=self._sub(val,1,8); self.r.cc &= ~C
        elif nib==12: val=self._add(val,1,8); self.r.cc &= ~C
        elif nib==13: self._nz(val,8);self.r.cc&=~V;return base
        elif nib==14:
            if ea is None: raise IllegalInstruction(start,op)
            self.r.pc=ea;return base-3
        elif nib==15: val=0;self.r.cc=(self.r.cc&~(N|V|C))|Z
        else: raise IllegalInstruction(start,op)
        if ea is None:setattr(self.r,target,val)
        else:self.wb(ea,val)
        return base

    def _alu(self, op: int) -> int|None:
        mode=(op>>4)&3; low=op&15; bank=op&0x40
        # mode 0 means immediate for 8x/Cx groups, otherwise direct/indexed/extended mapping
        amode=3 if mode==0 else mode-1
        reg="b" if bank else "a"
        if low in (0,1,2,4,5,6,8,9,10,11):
            value,x=self._operand(amode,8); left=getattr(self.r,reg)
            if low==0: out=self._sub(left,value,8)
            elif low==1: self._sub(left,value,8);return 2+mode+x
            elif low==2: out=self._sub(left,value,8,1 if self.r.cc&C else 0)
            elif low==4: out=self._logic(left&value,8)
            elif low==5: self._logic(left&value,8);return 2+mode+x
            elif low==6: out=self._logic(value,8)
            elif low==8: out=self._logic(left^value,8)
            elif low==9: out=self._add(left,value,8,1 if self.r.cc&C else 0)
            elif low==10: out=self._logic(left|value,8)
            else: out=self._add(left,value,8)
            setattr(self.r,reg,out);return 2+mode+x
        if low==7 and mode:
            ea,x=self._ea(mode-1);value=getattr(self.r,reg);self._logic(value,8);self.wb(ea,value);return 3+mode+x
        # 16-bit SUBD/ADDD, CMPX, LDX and B-side LDD/LDU
        if low in (3,12,14):
            dest = ("d" if low==3 else ("x" if not bank else "d") if low==12 else ("x" if not bank else "u"))
            val,x=self._operand(amode,16)
            if low==3: setattr(self.r,dest,self._sub(getattr(self.r,dest),val,16))
            elif low==12: self._sub(getattr(self.r,dest),val,16) if not bank else setattr(self.r,dest,self._logic(val,16))
            else: setattr(self.r,dest,self._logic(val,16))
            return (3 if mode==0 else 4+mode)+x
        if low==13 and not bank and mode: # JSR handled earlier
            return None
        if low==13 and bank: # STD
            if not mode:return None
            ea,x=self._ea(mode-1);self._logic(self.r.d,16);self.ww(ea,self.r.d);return 5+mode+x
        if low==15 and mode:
            ea,x=self._ea(mode-1);dest="x" if not bank else "u";v=getattr(self.r,dest);self._logic(v,16);self.ww(ea,v);return 5+mode+x
        return None

    def _page10(self, op:int)->int|None:
        if op in (0x83,0x8C,0x93,0x9C,0xA3,0xAC,0xB3,0xBC):
            mode=3 if op<0x90 else ((op>>4)&3)-1;v,x=self._operand(mode,16);self._sub(self.r.d if op&15==3 else self.r.y,v,16);return 5+((op>>4)&3)+x
        low=op&15; mode=(op>>4)&3
        if low in (0xC,0xE) and op>=0x80:
            am=3 if mode==0 else mode-1;v,x=self._operand(am,16);dest="y" if low==0xE or op<0xC0 else "s";setattr(self.r,dest,self._logic(v,16));return 4+mode+x
        if low==0xF and mode:
            ea,x=self._ea(mode-1);dest="y" if op<0xC0 else "s";v=getattr(self.r,dest);self._logic(v,16);self.ww(ea,v);return 6+mode+x
        return None

    def _page11(self, op:int)->int|None:
        if op in (0x83,0x8C,0x93,0x9C,0xA3,0xAC,0xB3,0xBC):
            mode=3 if op<0x90 else ((op>>4)&3)-1;v,x=self._operand(mode,16);self._sub(self.r.u if op&15==3 else self.r.s,v,16);return 5+((op>>4)&3)+x
        if self.variant=="6309": return self._hd6309(op)
        return None

    def _stack_op(self, op:int)->int:
        mask=self._fetch8(); stack="s" if op in (0x34,0x35) else "u";other="u" if stack=="s" else "s"; pull=op in (0x35,0x37)
        fields=[("cc",1),("a",1),("b",1),("dp",1),("x",2),("y",2),(other,2),("pc",2)]
        seq=fields if pull else list(reversed(fields));count=0
        for bit,(name,size) in zip(([1,2,4,8,16,32,64,128] if pull else [128,64,32,16,8,4,2,1]),seq):
            if mask&bit:
                if pull:setattr(self.r,name,self._pull8(stack) if size==1 else self._pull16(stack))
                else:(self._push8 if size==1 else self._push16)(stack,getattr(self.r,name))
                count+=size
        return 5+count

    def _reg(self, code:int)->tuple[str,int]:
        table={0:("d",16),1:("x",16),2:("y",16),3:("u",16),4:("s",16),5:("pc",16),8:("a",8),9:("b",8),10:("cc",8),11:("dp",8)}
        if self.variant=="6309":table.update({6:("w",16),7:("v",16),14:("e",8),15:("f",8)})
        if code not in table:raise IllegalInstruction((self.r.pc-2)&0xffff,code)
        return table[code]

    def _exg_tfr(self, exchange:bool)->int:
        post=self._fetch8();src,ss=self._reg(post>>4);dst,ds=self._reg(post&15)
        if ss!=ds: raise IllegalInstruction((self.r.pc-2)&0xffff,post)
        a,b=getattr(self.r,src),getattr(self.r,dst);setattr(self.r,dst,a)
        if exchange:setattr(self.r,src,b)
        return 8 if exchange else 6

    def _hd6309(self, op:int)->int|None:
        # Most useful native-register operations on page 11.
        if op in (0x3C,0x3D):
            if op==0x3C:self.r.md=self._fetch8()
            else:self.r.md &= ~self._fetch8()
            return 3
        # LDE/STE and LDF/STF use the same mode matrix as LDA/STA.
        if op>=0x80 and (op&15) in (6,7):
            mode=(op>>4)&3;reg="f" if op&0x40 else "e"
            if (op&15)==6:
                am=3 if mode==0 else mode-1;v,x=self._operand(am,8);setattr(self.r,reg,self._logic(v,8));return 3+mode+x
            if mode:
                ea,x=self._ea(mode-1);v=getattr(self.r,reg);self._logic(v,8);self.wb(ea,v);return 5+mode+x
        return None


class MC6809(CPU):
    def __init__(self, read: Read, write: Write): super().__init__(read, write, variant="6809")


class HD6309(CPU):
    def __init__(self, read: Read, write: Write): super().__init__(read, write, variant="6309")
