/* Bee-specific support for ELF.
   Copyright (C) 2020 Free Software Foundation, Inc.

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

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/bee.h"
#include "elfxx-bee.h"

#define MINUS_ONE ((bfd_vma)0 - 1)

/* The relocation table used for SHT_RELA sections.  */

static reloc_howto_type howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_BEE_NONE,		/* type */
	 0,			/* rightshift */
	 3,			/* size */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 32 bit absolute relocation.  */
  HOWTO (R_BEE_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_32",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 64 bit absolute relocation.  */
  HOWTO (R_BEE_64,		/* type */
	 0,			/* rightshift */
	 4,			/* size */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_64",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 30 bit absolute relocation.  */
  HOWTO (R_BEE_30,		/* type */
	 2,			/* rightshift */
	 2,			/* size */
	 30,			/* bitsize */
	 FALSE,			/* pc_relative */
	 2,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_30",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xfffffffc,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 61 bit absolute relocation.  */
  HOWTO (R_BEE_61,		/* type */
	 3,			/* rightshift */
	 4,			/* size */
	 61,			/* bitsize */
	 FALSE,			/* pc_relative */
	 3,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_61",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xfffffffffffffff8,	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* An 8 bit absolute relocation.  */
  HOWTO (R_BEE_8,		/* type */
	 0,			/* rightshift */
	 0,			/* size */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_8",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 30 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL30,		/* type */
	 2,			/* rightshift */
	 2,			/* size */
	 30,			/* bitsize */
	 TRUE,			/* pc_relative */
	 2,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_PCREL30",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xfffffffc,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* A 28 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL28,		/* type */
	 2,			/* rightshift */
	 2,			/* size */
	 28,			/* bitsize */
	 TRUE,			/* pc_relative */
	 4,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_PCREL28",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xfffffffc,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* A 61 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL61,		/* type */
	 3,			/* rightshift */
	 4,			/* size */
	 61,			/* bitsize */
	 TRUE,			/* pc_relative */
	 3,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_PCREL61",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xfffffffffffffff8,	/* dst_mask */
	 TRUE),			/* pcrel_offset */
};

/* Map BFD reloc types to BEE ELF reloc types.  */

struct bee_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned int bee_reloc_val;
};

static const struct bee_reloc_map bee_reloc_map [] =
{
  { BFD_RELOC_NONE,	       R_BEE_NONE },
  { BFD_RELOC_32,	       R_BEE_32 },
  { BFD_RELOC_64,              R_BEE_64 },
  { BFD_RELOC_BEE_30,	       R_BEE_30 },
  { BFD_RELOC_BEE_61,	       R_BEE_61 },
  { BFD_RELOC_8,	       R_BEE_8 },
  { BFD_RELOC_BEE_30_PCREL,    R_BEE_PCREL30 },
  { BFD_RELOC_BEE_28_PCREL,    R_BEE_PCREL28 },
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
      return & howto_table [bee_reloc_map[i].bee_reloc_val];

  return NULL;
}

reloc_howto_type *
bee_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (howto_table) / sizeof (howto_table[0]);
       i++)
    if (howto_table[i].name != NULL
	&& strcasecmp (howto_table[i].name, r_name) == 0)
      return &howto_table[i];

  return NULL;
}

/* Set the howto pointer for an BEE ELF reloc.  */

reloc_howto_type *
bee_elf_rtype_to_howto (bfd *abfd,
                        unsigned int r_type)
{
  if (r_type >= (unsigned int) R_BEE_max)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			  abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }
  return &howto_table [r_type];
}
