// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from github.com/omikron88/ondra by the same project author.
// Z80 opcode fields decoded independently of the CPU execution engine.
#include "vz256/disassembler.hpp"
#include "vz256/machine.hpp"
#include <cstdio>

namespace vz256 {
namespace {
std::string hex(unsigned v, unsigned digits = 2) { char text[16]; std::snprintf(text,sizeof(text),"%0*Xh",digits,v); return text; }
const char* conditions[] = {"NZ","Z","NC","C","PO","PE","P","M"};
const char* alu[] = {"ADD A,","ADC A,","SUB ","SBC A,","AND ","XOR ","OR ","CP "};
}
Instruction disassemble(Machine& m, std::uint16_t address) {
    unsigned length = 0;
    auto byte = [&]() { return unsigned(m.read(static_cast<std::uint16_t>(address + length++))); };
    auto word = [&]() { const unsigned lo = byte(); return lo | (byte() << 8); };
    unsigned op = byte(); std::string index = "HL";
    while ((op == 0xdd || op == 0xfd) && length < 16) { index = op == 0xdd ? "IX" : "IY"; op = byte(); }
    const bool indexed = index != "HL";
    if (op == 0xdd || op == 0xfd) return Instruction{"DB " + hex(op) + " ; repeated prefixes",length};
    const unsigned x = op >> 6, y = (op >> 3) & 7, z = op & 7, p = y >> 1, q = y & 1;
    std::string displacement;
    auto memory = [&]() {
        if (!indexed) return std::string("(HL)");
        if (displacement.empty()) { const int d = static_cast<std::int8_t>(byte()); displacement = "(" + index + (d < 0 ? "-" : "+") + hex(d < 0 ? -d : d) + ")"; }
        return displacement;
    };
    auto reg = [&](unsigned r, bool halves) {
        const char* names[] = {"B","C","D","E","H","L","(HL)","A"};
        if (r == 6) return memory();
        if (indexed && halves && (r == 4 || r == 5)) return index + (r == 4 ? "H" : "L");
        return std::string(names[r]);
    };
    auto pair = [&](unsigned r, bool af) {
        if (r == 2) return index;
        return std::string(r == 0 ? "BC" : r == 1 ? "DE" : af ? "AF" : "SP");
    };
    auto relative = [&]() { const int d = static_cast<std::int8_t>(byte()); return hex((address + length + d) & 65535,4); };
    std::string text;
    if (op == 0xcb) {
        std::string operand;
        if (indexed) operand = memory();
        const unsigned cb = byte(), group = cb >> 6, bit = (cb >> 3) & 7, r = cb & 7;
        if (!indexed) operand = reg(r,true);
        const char* shifts[] = {"RLC ","RRC ","RL ","RR ","SLA ","SRA ","SLL ","SRL "};
        if (!group) text = shifts[bit] + operand;
        else text = std::string(group == 1 ? "BIT " : group == 2 ? "RES " : "SET ") + std::to_string(bit) + "," + operand;
        if (indexed && group != 1 && r != 6) { const char* names[] = {"B","C","D","E","H","L","","A"}; text += "," + std::string(names[r]); }
    } else if (op == 0xed) {
        index = "HL"; // DD/FD before ED is ignored.
        const unsigned ed = byte(), ex = ed >> 6, ey = (ed >> 3) & 7, ez = ed & 7, ep = ey >> 1, eq = ey & 1;
        const char* registers[] = {"B","C","D","E","H","L","0","A"};
        if (ex == 1) {
            switch (ez) {
            case 0: text = ey == 6 ? "IN (C)" : "IN " + std::string(registers[ey]) + ",(C)"; break;
            case 1: text = "OUT (C)," + std::string(registers[ey]); break;
            case 2: text = std::string(eq ? "ADC HL," : "SBC HL,") + pair(ep,false); break;
            case 3: { const std::string nn = "(" + hex(word(),4) + ")"; text = "LD " + (eq ? pair(ep,false) + "," + nn : nn + "," + pair(ep,false)); break; }
            case 4: text = "NEG"; break;
            case 5: text = ed == 0x4d ? "RETI" : "RETN"; break;
            case 6: { const unsigned modes[] = {0,0,1,2,0,0,1,2}; text = "IM " + std::to_string(modes[ey]); break; }
            case 7: { const char* names[] = {"LD I,A","LD R,A","LD A,I","LD A,R","RRD","RLD","NOP","NOP"}; text = names[ey]; break; }
            }
        } else if (ex == 2 && ey >= 4 && ez <= 3) {
            const char* names[4][4] = {{"LDI","CPI","INI","OUTI"},{"LDD","CPD","IND","OUTD"},{"LDIR","CPIR","INIR","OTIR"},{"LDDR","CPDR","INDR","OTDR"}};
            text = names[ey - 4][ez];
        } else text = "NOP ; ED " + hex(ed);
    } else if (x == 0) {
        switch (z) {
        case 0:
            if (y == 0) text = "NOP";
            else if (y == 1) text = "EX AF,AF'";
            else if (y == 2) text = "DJNZ " + relative();
            else if (y == 3) text = "JR " + relative();
            else text = "JR " + std::string(conditions[y - 4]) + "," + relative();
            break;
        case 1: text = q ? "ADD " + index + "," + pair(p,false) : "LD " + pair(p,false) + "," + hex(word(),4); break;
        case 2: {
            const std::string place = p < 2 ? (p ? "(DE)" : "(BC)") : "(" + hex(word(),4) + ")";
            const std::string value = p == 2 ? index : "A";
            text = "LD " + (q ? value + "," + place : place + "," + value); break;
        }
        case 3: text = std::string(q ? "DEC " : "INC ") + pair(p,false); break;
        case 4: text = "INC " + reg(y,true); break;
        case 5: text = "DEC " + reg(y,true); break;
        case 6: { const std::string dest = reg(y,true); text = "LD " + dest + "," + hex(byte()); break; }
        case 7: { const char* names[] = {"RLCA","RRCA","RLA","RRA","DAA","CPL","SCF","CCF"}; text = names[y]; break; }
        }
    } else if (x == 1) {
        if (op == 0x76) text = "HALT";
        else { const std::string dest = reg(y,z != 6); const std::string source = reg(z,y != 6); text = "LD " + dest + "," + source; }
    } else if (x == 2) text = alu[y] + reg(z,true);
    else {
        switch (z) {
        case 0: text = "RET " + std::string(conditions[y]); break;
        case 1:
            if (!q) text = "POP " + pair(p,true);
            else if (p == 0) text = "RET";
            else if (p == 1) text = "EXX";
            else if (p == 2) text = "JP (" + index + ")";
            else text = "LD SP," + index;
            break;
        case 2: text = "JP " + std::string(conditions[y]) + "," + hex(word(),4); break;
        case 3:
            if (y == 0) text = "JP " + hex(word(),4);
            else if (y == 2) text = "OUT (" + hex(byte()) + "),A";
            else if (y == 3) text = "IN A,(" + hex(byte()) + ")";
            else if (y == 4) text = "EX (SP)," + index;
            else if (y == 5) text = "EX DE,HL";
            else text = y == 6 ? "DI" : "EI";
            break;
        case 4: text = "CALL " + std::string(conditions[y]) + "," + hex(word(),4); break;
        case 5: text = !q ? "PUSH " + pair(p,true) : "CALL " + hex(word(),4); break;
        case 6: text = alu[y] + hex(byte()); break;
        case 7: text = "RST " + hex(y * 8); break;
        }
    }
    return Instruction{text,length};
}
}
