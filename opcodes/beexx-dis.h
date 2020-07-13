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
#include <stdio.h>

#define STATIC_TABLE
#define DEFINE_TABLE

#include "opcode/bee.h"
#include "disassemble.h"

static fprintf_ftype fpr;
static void *stream;

extern const bee_opc_info_t BEENN_(_inst_info)[0x40];

int
_NN(print_insn_bee) (bfd_vma addr, struct disassemble_info * info);

int
_NN(print_insn_bee) (bfd_vma addr, struct disassemble_info * info)
{
  int length = BEE_WORD_BYTES;
  int status;
  stream = info->stream;
  const bee_opc_info_t * opcode;
  bfd_byte buffer[BEE_WORD_BYTES];
  uintptr_t iword;
  fpr = info->fprintf_func;

  if ((status = info->read_memory_func (addr, buffer, BEE_WORD_BYTES, info)))
    goto fail;

  if (info->endian == BFD_ENDIAN_BIG)
    iword = _NN(bfd_getb) (buffer);
  else
    iword = _NN(bfd_getl) (buffer);

  switch (iword & BEE_OP1_MASK)
    {
    case BEE_OP_CALLI:
      fpr (stream, "calli\t0x%lx", addr + (bfd_signed_vma)(int)(iword >> BEE_OP1_SHIFT) * BEE_WORD_BYTES);
      break;
    case BEE_OP_PUSHI:
      fpr (stream, "pushi\t0x%x # %d", (int)iword >> 2, (int)iword >> BEE_OP1_SHIFT);
      break;
    case BEE_OP_PUSHRELI:
      fpr (stream, "pushreli\t0x%lx", addr + (bfd_signed_vma)(int)(iword >> BEE_OP1_SHIFT) * BEE_WORD_BYTES);
      break;
    default: /* second-level instruction */
      switch (iword & BEE_OP2_MASK)
        {
        case BEE_OP_JUMPI:
          fpr (stream, "jumpi\t0x%lx", addr + (bfd_signed_vma)(int)(iword >> BEE_OP2_SHIFT) * BEE_WORD_BYTES);
          break;
        case BEE_OP_JUMPZI:
          fpr (stream, "jumpzi\t0x%lx", addr + (bfd_signed_vma)(int)(iword >> BEE_OP2_SHIFT) * BEE_WORD_BYTES);
          break;
        case BEE_OP_TRAP:
          fpr (stream, "trap\t0x%lx", (bfd_size_type)iword >> BEE_OP2_SHIFT);
          break;
        case BEE_OP_INSN:
          {
            unsigned raw_opcode = iword >> BEE_OP2_SHIFT;
            if (raw_opcode <= sizeof (BEENN_(_inst_info)) / sizeof (BEENN_(_inst_info)[0]))
              {
                opcode = &BEENN_(_inst_info)[raw_opcode];
                if (strcmp(opcode->name, ""))
                  fpr (stream, "%s", opcode->name);
                else
                  fpr (stream, "; invalid instruction!");
              }
          }
          break;
        }
    }

  return length;

 fail:
  info->memory_error_func (status, addr, info);
  return -1;
}
