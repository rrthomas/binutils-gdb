/* Bee-specific support for ELF.
   Copyright (C) 2009-2020 Free Software Foundation, Inc.

   Copied from elf32-moxie.c, which is
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

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/bee.h"
#include "bee.h"

#define MINUS_ONE (((bfd_vma)0) - 1)

reloc_howto_type bee_elf_howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_BEE_NONE,		/* type */
         0,			/* rightshift */
         3,			/* size (0 = byte, 1 = short, 2 = long) */
         0,			/* bitsize */
         false,			/* pc_relative */
         0,			/* bitpos */
         complain_overflow_dont, /* complain_on_overflow */
         bfd_elf_generic_reloc,	/* special_function */
         "R_BEE_NONE",		/* name */
         false,			/* partial_inplace */
         0,			/* src_mask */
         0,			/* dst_mask */
         false),		/* pcrel_offset */

  /* A 32 bit absolute relocation.  */
  HOWTO (R_BEE_32,		/* type */
         0,			/* rightshift */
         2,			/* size (0 = byte, 1 = short, 2 = long) */
         32,			/* bitsize */
         false,			/* pc_relative */
         0,			/* bitpos */
         complain_overflow_bitfield, /* complain_on_overflow */
         bfd_elf_generic_reloc,	/* special_function */
         "R_BEE_32",		/* name */
         false,			/* partial_inplace */
         0x00000000,		/* src_mask */
         0xffffffff,		/* dst_mask */
         false),		/* pcrel_offset */

  /* A 30 bit absolute relocation.  */
  HOWTO (R_BEE_30,	/* type.  */
         2,			/* rightshift.  */
         2,			/* size (0 = byte, 1 = short, 2 = long).  */
         30,			/* bitsize.  */
         false,			/* pc_relative.  */
         2,			/* bitpos.  */
         complain_overflow_signed, /* complain_on_overflow.  */
         bfd_elf_generic_reloc,	/* special_function.  */
         "R_BEE_30",		/* name.  */
         false,			/* partial_inplace.  */
         0,			/* src_mask.  */
         0xfffffffc,		/* dst_mask.  */
         false),		/* pcrel_offset.  */

  /* A 30 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL30,	/* type.  */
         2,			/* rightshift.  */
         2,			/* size (0 = byte, 1 = short, 2 = long).  */
         30,			/* bitsize.  */
         true,			/* pc_relative.  */
         2,			/* bitpos.  */
         complain_overflow_signed, /* complain_on_overflow.  */
         bfd_elf_generic_reloc,	/* special_function.  */
         "R_BEE_PCREL30",	/* name.  */
         false,			/* partial_inplace.  */
         0,			/* src_mask.  */
         0xfffffffc,		/* dst_mask.  */
         true),			/* pcrel_offset.  */

  /* A 28 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL28,	/* type.  */
         2,			/* rightshift.  */
         2,			/* size (0 = byte, 1 = short, 2 = long).  */
         28,			/* bitsize.  */
         true,			/* pc_relative.  */
         4,			/* bitpos.  */
         complain_overflow_signed, /* complain_on_overflow.  */
         bfd_elf_generic_reloc,	/* special_function.  */
         "R_BEE_PCREL28",	/* name.  */
         false,			/* partial_inplace.  */
         0,			/* src_mask.  */
         0xfffffffc,		/* dst_mask.  */
         true),			/* pcrel_offset.  */

  /* A 64 bit absolute relocation.  */
  HOWTO (R_BEE_64,              /* type */
         0,                     /* rightshift */
         4,                     /* size */
         64,                    /* bitsize */
         false,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield, /* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_BEE_64",            /* name */
         false,                 /* partial_inplace */
         0,                     /* src_mask */
         MINUS_ONE,             /* dst_mask */
         false),                /* pcrel_offset */

  /* A 61 bit absolute relocation.  */
  HOWTO (R_BEE_61,              /* type */
         3,                     /* rightshift */
         4,                     /* size */
         61,                    /* bitsize */
         false,                 /* pc_relative */
         3,                     /* bitpos */
         complain_overflow_signed, /* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_BEE_61",            /* name */
         false,                 /* partial_inplace */
         0,                     /* src_mask */
         0xfffffffffffffff8,    /* dst_mask */
         false),                /* pcrel_offset */

  /* An 8 bit absolute relocation.  */
  HOWTO (R_BEE_8,	/* type.  */
         0,			/* rightshift.  */
         0,			/* size (0 = byte, 1 = short, 2 = long).  */
         8,			/* bitsize.  */
         false,			/* pc_relative.  */
         0,			/* bitpos.  */
         complain_overflow_signed, /* complain_on_overflow.  */
         bfd_elf_generic_reloc,	/* special_function.  */
         "R_BEE_8",		/* name.  */
         false,			/* partial_inplace.  */
         0,			/* src_mask.  */
         0xff,			/* dst_mask.  */
         false),		/* pcrel_offset.  */

  /* A 61 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL61,         /* type */
         3,                     /* rightshift */
         4,                     /* size */
         61,                    /* bitsize */
         true,                  /* pc_relative */
         3,                     /* bitpos */
         complain_overflow_signed, /* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_BEE_PCREL61",       /* name */
         false,                 /* partial_inplace */
         0,                     /* src_mask */
         0xfffffffffffffff8,    /* dst_mask */
         true),                 /* pcrel_offset */
};

const struct bee_reloc_map bee_reloc_map [] =
{
  { BFD_RELOC_NONE,	       R_BEE_NONE },
  { BFD_RELOC_32,	       R_BEE_32 },
  { BFD_RELOC_BEE_30,	       R_BEE_30 },
  { BFD_RELOC_8,	       R_BEE_8 },
  { BFD_RELOC_BEE_30_PCREL,    R_BEE_PCREL30 },
  { BFD_RELOC_BEE_28_PCREL,    R_BEE_PCREL28 },
  { BFD_RELOC_64,	       R_BEE_64 },
  { BFD_RELOC_BEE_61,	       R_BEE_61 },
  { BFD_RELOC_BEE_61_PCREL,    R_BEE_PCREL61 },
};

reloc_howto_type *
bee_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
                         bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = sizeof (bee_reloc_map) / sizeof (bee_reloc_map[0]);
       i--;)
    if (bee_reloc_map [i].bfd_reloc_val == code)
      return & bee_elf_howto_table [bee_reloc_map[i].bee_reloc_val];

  return NULL;
}

reloc_howto_type *
bee_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (bee_elf_howto_table) / sizeof (bee_elf_howto_table[0]);
       i++)
    if (bee_elf_howto_table[i].name != NULL
        && strcasecmp (bee_elf_howto_table[i].name, r_name) == 0)
      return &bee_elf_howto_table[i];

  return NULL;
}
