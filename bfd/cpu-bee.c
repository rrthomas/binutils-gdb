/* BFD support for the Bee virtual machine.
   Copyright (C) 2020 Free Software Foundation, Inc.
   Written by Reuben Thomas

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

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"


const bfd_arch_info_type bfd_bee32_arch =
{
  32,		      /* Bits in a word.  */
  32,		      /* Bits in an address.  */
  8,		      /* Bits in a byte.  */
  bfd_arch_bee,       /* Architecture Number.  */
  bfd_mach_bee32,     /* Machine number.  */
  "bee",	      /* Arch name.  */
  "bee:32",	      /* Printable name.  */
  2,		      /* Section alignment power.  */
  false,	      /* Not the default.  */
  bfd_default_compatible,
  bfd_default_scan,
  bfd_arch_default_fill,
  NULL,
  0 /* Maximum offset of a reloc from the start of an insn.  */
};

const bfd_arch_info_type bfd_bee_arch =
{
  64,		      /* Bits in a word.  */
  64,		      /* Bits in an address.  */
  8,		      /* Bits in a byte.  */
  bfd_arch_bee,       /* Architecture Number.  */
  bfd_mach_bee64,     /* Machine number.  */
  "bee",	      /* Arch name.  */
  "bee:64",	      /* Printable name.  */
  3,		      /* Section alignment power.  */
  true,		      /* The default.  */
  bfd_default_compatible,
  bfd_default_scan,
  bfd_arch_default_fill,
  &bfd_bee32_arch,
  0 /* Maximum offset of a reloc from the start of an insn.  */
};
