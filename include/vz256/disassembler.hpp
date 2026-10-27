// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from github.com/omikron88/ondra by the same project author.
#pragma once
#include <cstdint>
#include <string>
namespace vz256 {
class Machine;
struct Instruction { std::string text; unsigned length; };
Instruction disassemble(Machine& machine, std::uint16_t address);
}
