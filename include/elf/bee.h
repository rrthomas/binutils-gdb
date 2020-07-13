/* Bee ELF support for BFD.
   Copyright (C) 2020 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _ELF_BEE_H
#define _ELF_BEE_H

#include "elf/reloc-macros.h"

/* Relocation types.  */
START_RELOC_NUMBERS (elf_bee_reloc_type)
  RELOC_NUMBER (R_BEE_NONE, 0)

  /* 32- and 64-bit.  */
  RELOC_NUMBER (R_BEE_8, 1)

  /* 32-bit.  */
  RELOC_NUMBER (R_BEE_32, 2)
  RELOC_NUMBER (R_BEE_30, 3)
  RELOC_NUMBER (R_BEE_PCREL30, 4)
  RELOC_NUMBER (R_BEE_PCREL28, 5)

  /* 64-bit.  */
  RELOC_NUMBER (R_BEE_64, 6)
  RELOC_NUMBER (R_BEE_61, 7)
  RELOC_NUMBER (R_BEE_PCREL61, 8)
END_RELOC_NUMBERS (R_BEE_max)

#endif /* _ELF_BEE32_H */
