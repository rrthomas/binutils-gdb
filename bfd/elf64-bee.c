/* Bee-specific support for 64-bit ELF.
   Copyright (C) 2009-2020 Free Software Foundation, Inc.

   Copied from elf32-moxie.c, and elfnn-riscv.c which are
   Copyright (C) 1998-2020 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#define ELFNN_R_TYPE(x) ELF64_R_TYPE(x)
#define ELFNN_R_SYM(x) ELF64_R_SYM(x)

#include "elfxx-bee.h"

#define TARGET_BIG_SYM		bee_elf64_be_vec
#define TARGET_BIG_NAME		"elf64-bigbee"
#define TARGET_LITTLE_SYM	bee_elf64_le_vec
#define TARGET_LITTLE_NAME	"elf64-littlebee"

#define bfd_elf64_bfd_reloc_type_lookup		bee_reloc_type_lookup
#define bfd_elf64_bfd_reloc_name_lookup		bee_reloc_name_lookup

#include "elf64-target.h"
