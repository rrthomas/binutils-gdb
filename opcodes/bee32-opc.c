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

#define BEE_WORD_BYTES 4
#define BEENN_(name) bee32 ## name
#include "beexx-opc.h"

/* The Bee virtual machine's 32-bit instructions come in four forms:

  CALL has two low bits 00, and the top 30 bits are a PC-relative offset in
  words.

  PUSH has two low bits 01, and the top 30 bits are a signed constant.

  PUSHREL has two low bits 10, and the top 30 bits are as for CALL.

  Other instructions have two low bits 11, and the next 8 bits are the
  instruction opcode. If the top opcode bit is set, the instruction is an
  implementation-dependent extra instruction; otherwise, a core instruction
  that can be (dis)assembled.
*/
