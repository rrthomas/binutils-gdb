/* Disassemble Bee instructions.
   Copyright (C) 2020 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "disassemble.h"

int
print_insn_bee32 (bfd_vma addr, struct disassemble_info * info);
int
print_insn_bee64 (bfd_vma addr, struct disassemble_info * info);

int
print_insn_bee (bfd_vma addr, struct disassemble_info * info)
{
  if (info->mach == bfd_mach_bee32)
    return print_insn_bee32 (addr, info);
  else if (info->mach == bfd_mach_bee64)
    return print_insn_bee64 (addr, info);
  else
    return -1;
}
