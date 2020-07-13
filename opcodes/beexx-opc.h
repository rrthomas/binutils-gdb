/* Definitions for Bee opcodes.

   Copyright (C) 2020 Free Software Foundation, Inc.
   Contributed by Reuben Thomas <rrt@sc3d.org>.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "opcode/bee.h"

const bee_opc_info_t BEENN_(_opc_info)[16] =
  {
   { BEE_OP_CALLI,    "calli" },
   { BEE_OP_PUSHI,    "pushi" },
   { BEE_OP_PUSHRELI, "pushreli" },
   { BEE_OP_JUMPI,    "jumpi" },
   { 0, "" },
   { 0, "" },
   { 0, "" },
   { BEE_OP_JUMPZI,   "jumpzi" },
   { 0, "" },
   { 0, "" },
   { 0, "" },
   { BEE_OP_TRAP,     "trap" },
   { 0, "" },
   { 0, "" },
   { 0, "" },
   { BEE_OP_INSN,     "" },
  };

const bee_opc_info_t BEENN_(_inst_info)[0x40] =
  {
    { (BEE_INSN_NOP << BEE_OP2_SHIFT) | BEE_OP_INSN,        "nop" },
    { (BEE_INSN_NOT << BEE_OP2_SHIFT) | BEE_OP_INSN,        "not" },
    { (BEE_INSN_AND << BEE_OP2_SHIFT) | BEE_OP_INSN,        "and" },
    { (BEE_INSN_OR << BEE_OP2_SHIFT) | BEE_OP_INSN,         "or" },
    { (BEE_INSN_XOR << BEE_OP2_SHIFT) | BEE_OP_INSN,        "xor" },
    { (BEE_INSN_LSHIFT << BEE_OP2_SHIFT) | BEE_OP_INSN,     "lshift" },
    { (BEE_INSN_RSHIFT << BEE_OP2_SHIFT) | BEE_OP_INSN,     "rshift" },
    { (BEE_INSN_ARSHIFT << BEE_OP2_SHIFT) | BEE_OP_INSN,    "arshift" },
    { (BEE_INSN_POP << BEE_OP2_SHIFT) | BEE_OP_INSN,        "pop" },
    { (BEE_INSN_DUP << BEE_OP2_SHIFT) | BEE_OP_INSN,        "dup" },
    { (BEE_INSN_SET << BEE_OP2_SHIFT) | BEE_OP_INSN,        "set" },
    { (BEE_INSN_SWAP << BEE_OP2_SHIFT) | BEE_OP_INSN,       "swap" },
    { (BEE_INSN_JUMP << BEE_OP2_SHIFT) | BEE_OP_INSN,       "jump" },
    { (BEE_INSN_JUMPZ << BEE_OP2_SHIFT) | BEE_OP_INSN,      "jumpz" },
    { (BEE_INSN_CALL << BEE_OP2_SHIFT) | BEE_OP_INSN,       "call" },
    { (BEE_INSN_RET << BEE_OP2_SHIFT) | BEE_OP_INSN,        "ret" },
    { (BEE_INSN_LOAD << BEE_OP2_SHIFT) | BEE_OP_INSN,       "load" },
    { (BEE_INSN_STORE << BEE_OP2_SHIFT) | BEE_OP_INSN,      "store" },
    { (BEE_INSN_LOAD1 << BEE_OP2_SHIFT) | BEE_OP_INSN,      "load1" },
    { (BEE_INSN_STORE1 << BEE_OP2_SHIFT) | BEE_OP_INSN,     "store1" },
    { (BEE_INSN_LOAD2 << BEE_OP2_SHIFT) | BEE_OP_INSN,      "load2" },
    { (BEE_INSN_STORE2 << BEE_OP2_SHIFT) | BEE_OP_INSN,     "store2" },
    { (BEE_INSN_LOAD4 << BEE_OP2_SHIFT) | BEE_OP_INSN,      "load4" },
    { (BEE_INSN_STORE4 << BEE_OP2_SHIFT) | BEE_OP_INSN,     "store4" },
    { (BEE_INSN_LOAD_IA << BEE_OP2_SHIFT) | BEE_OP_INSN,    "load_ia" },
    { (BEE_INSN_STORE_DB << BEE_OP2_SHIFT) | BEE_OP_INSN,   "store_db" },
    { (BEE_INSN_LOAD_IB << BEE_OP2_SHIFT) | BEE_OP_INSN,    "load_ib" },
    { (BEE_INSN_STORE_DA << BEE_OP2_SHIFT) | BEE_OP_INSN,   "store_da" },
    { (BEE_INSN_LOAD_DA << BEE_OP2_SHIFT) | BEE_OP_INSN,    "load_da" },
    { (BEE_INSN_STORE_IB << BEE_OP2_SHIFT) | BEE_OP_INSN,   "store_ib" },
    { (BEE_INSN_LOAD_DB << BEE_OP2_SHIFT) | BEE_OP_INSN,    "load_db" },
    { (BEE_INSN_STORE_IA << BEE_OP2_SHIFT) | BEE_OP_INSN,   "store_ia" },
    { (BEE_INSN_NEG << BEE_OP2_SHIFT) | BEE_OP_INSN,        "neg" },
    { (BEE_INSN_ADD << BEE_OP2_SHIFT) | BEE_OP_INSN,        "add" },
    { (BEE_INSN_MUL << BEE_OP2_SHIFT) | BEE_OP_INSN,        "mul" },
    { (BEE_INSN_DIVMOD << BEE_OP2_SHIFT) | BEE_OP_INSN,     "divmod" },
    { (BEE_INSN_UDIVMOD << BEE_OP2_SHIFT) | BEE_OP_INSN,    "udivmod" },
    { (BEE_INSN_EQ << BEE_OP2_SHIFT) | BEE_OP_INSN,         "eq" },
    { (BEE_INSN_LT << BEE_OP2_SHIFT) | BEE_OP_INSN,         "lt" },
    { (BEE_INSN_ULT << BEE_OP2_SHIFT) | BEE_OP_INSN,        "ult" },
    { (BEE_INSN_PUSHS << BEE_OP2_SHIFT) | BEE_OP_INSN,      "pushs" },
    { (BEE_INSN_POPS << BEE_OP2_SHIFT) | BEE_OP_INSN,       "pops" },
    { (BEE_INSN_DUPS << BEE_OP2_SHIFT) | BEE_OP_INSN,       "dups" },
    { (BEE_INSN_CATCH << BEE_OP2_SHIFT) | BEE_OP_INSN,      "catch" },
    { (BEE_INSN_THROW << BEE_OP2_SHIFT) | BEE_OP_INSN,      "throw" },
    { (BEE_INSN_BREAK << BEE_OP2_SHIFT) | BEE_OP_INSN,      "break" },
    { (BEE_INSN_WORD_BYTES << BEE_OP2_SHIFT) | BEE_OP_INSN, "word_bytes" },
    { (BEE_INSN_GET_SSIZE << BEE_OP2_SHIFT) | BEE_OP_INSN,  "get_ssize" },
    { (BEE_INSN_GET_SP << BEE_OP2_SHIFT) | BEE_OP_INSN,     "get_sp" },
    { (BEE_INSN_SET_SP << BEE_OP2_SHIFT) | BEE_OP_INSN,     "set_sp" },
    { (BEE_INSN_GET_DSIZE << BEE_OP2_SHIFT) | BEE_OP_INSN,  "get_dsize" },
    { (BEE_INSN_GET_DP << BEE_OP2_SHIFT) | BEE_OP_INSN,     "get_dp" },
    { (BEE_INSN_SET_DP << BEE_OP2_SHIFT) | BEE_OP_INSN,     "set_dp" },
    { (BEE_INSN_GET_HANDLER_SP << BEE_OP2_SHIFT) | BEE_OP_INSN, "get_handler_sp" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
    { 0,                   "" },
  };
