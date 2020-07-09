/* Bee-specific support for 32-bit ELF.
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

/* Forward declarations.  */

static reloc_howto_type bee_elf_howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_BEE_NONE,		/* type */
	 0,			/* rightshift */
	 3,			/* size (0 = byte, 1 = short, 2 = long) */
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
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_BEE_32",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 30 bit absolute relocation.  */
  HOWTO (R_BEE_30,	/* type.  */
	 2,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 30,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 2,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_BEE_30",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xfffffffc,		/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  /* An 8 bit absolute relocation.  */
  HOWTO (R_BEE_8,	/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 8,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_BEE_8",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xff,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  /* A 30 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL30,	/* type.  */
	 2,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 30,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 2,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_BEE_PCREL30",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xfffffffc,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  /* A 28 bit PC-relative relocation.  */
  HOWTO (R_BEE_PCREL28,	/* type.  */
	 2,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 28,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 4,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_BEE_PCREL28",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xfffffffc,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */
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
  { BFD_RELOC_BEE_30,	       R_BEE_30 },
  { BFD_RELOC_8,	       R_BEE_8 },
  { BFD_RELOC_BEE_30_PCREL,    R_BEE_PCREL30 },
  { BFD_RELOC_BEE_28_PCREL,    R_BEE_PCREL28 },
};

static reloc_howto_type *
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

static reloc_howto_type *
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

/* Set the howto pointer for an BEE ELF reloc.  */

static bfd_boolean
bee_info_to_howto_rela (bfd *abfd,
			  arelent *cache_ptr,
			  Elf_Internal_Rela *dst)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  if (r_type >= (unsigned int) R_BEE_max)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			  abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }
  cache_ptr->howto = & bee_elf_howto_table [r_type];
  return TRUE;
}

/* Perform a single relocation.  We use the standard BFD routines.  */

static bfd_reloc_status_type
bee_final_link_relocate (reloc_howto_type *howto,
			   bfd *input_bfd,
			   asection *input_section,
			   bfd_byte *contents,
			   Elf_Internal_Rela *rel,
			   bfd_vma relocation)
{
  bfd_reloc_status_type r = bfd_reloc_ok;

  switch (howto->type)
    {
    default:
      r = _bfd_final_link_relocate (howto, input_bfd, input_section,
				    contents, rel->r_offset,
				    relocation, rel->r_addend);
    }

  return r;
}

/* Relocate an BEE ELF section.

   The RELOCATE_SECTION function is called by the new ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures; if the section
   actually uses Rel structures, the r_addend field will always be
   zero.

   This function is responsible for adjusting the section contents as
   necessary, and (if using Rela relocs and generating a relocatable
   output file) adjusting the reloc addend as necessary.

   This function does not have to worry about setting the reloc
   address or the reloc symbol index.

   LOCAL_SYMS is a pointer to the swapped in local symbols.

   LOCAL_SECTIONS is an array giving the section in the input file
   corresponding to the st_shndx field of each local symbol.

   The global hash table entry for the global symbols can be found
   via elf_sym_hashes (input_bfd).

   When generating relocatable output, this function must handle
   STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
   going to be the section symbol corresponding to the output
   section, which means that the addend must be adjusted
   accordingly.  */

static bfd_boolean
bee_elf_relocate_section (bfd *output_bfd,
			    struct bfd_link_info *info,
			    bfd *input_bfd,
			    asection *input_section,
			    bfd_byte *contents,
			    Elf_Internal_Rela *relocs,
			    Elf_Internal_Sym *local_syms,
			    asection **local_sections)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *howto;
      unsigned long r_symndx;
      Elf_Internal_Sym *sym;
      asection *sec;
      struct elf_link_hash_entry *h;
      bfd_vma relocation;
      bfd_reloc_status_type r;
      const char *name;
      int r_type;

      r_type = ELF32_R_TYPE (rel->r_info);
      r_symndx = ELF32_R_SYM (rel->r_info);
      howto  = bee_elf_howto_table + r_type;
      h      = NULL;
      sym    = NULL;
      sec    = NULL;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);

	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = name == NULL ? bfd_section_name (sec) : name;
	}
      else
	{
	  bfd_boolean unresolved_reloc, warned, ignored;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);

	  name = h->root.root.string;
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (bfd_link_relocatable (info))
	continue;

      r = bee_final_link_relocate (howto, input_bfd, input_section,
				     contents, rel, relocation);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      (*info->callbacks->reloc_overflow)
		(info, (h ? &h->root : NULL), name, howto->name,
		 (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	      break;

	    case bfd_reloc_undefined:
	      (*info->callbacks->undefined_symbol)
		(info, name, input_bfd, input_section, rel->r_offset, TRUE);
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      break;

	    case bfd_reloc_notsupported:
	      msg = _("internal error: unsupported relocation error");
	      break;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous relocation");
	      break;

	    default:
	      msg = _("internal error: unknown error");
	      break;
	    }

	  if (msg)
	    (*info->callbacks->warning) (info, msg, name, input_bfd,
					 input_section, rel->r_offset);
	}
    }

  return TRUE;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
bee_elf_gc_mark_hook (asection *sec,
			struct bfd_link_info *info,
			Elf_Internal_Rela *rel,
			struct elf_link_hash_entry *h,
			Elf_Internal_Sym *sym)
{
  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Look through the relocs for a section during the first phase.
   Since we don't do .gots or .plts, we just need to consider the
   virtual table relocs for gc.  */

static bfd_boolean
bee_elf_check_relocs (bfd *abfd,
			struct bfd_link_info *info,
			asection *sec,
			const Elf_Internal_Rela *relocs)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;

  if (bfd_link_relocatable (info))
    return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);

  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry *h;
      unsigned long r_symndx;

      r_symndx = ELF32_R_SYM (rel->r_info);
      if (r_symndx < symtab_hdr->sh_info)
	h = NULL;
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}
    }

  return TRUE;
}

#define ELF_ARCH		bfd_arch_bee
#define ELF_MACHINE_CODE	EM_BEE
#define ELF_MAXPAGESIZE		0x1

#define TARGET_BIG_SYM		bee_elf32_be_vec
#define TARGET_BIG_NAME		"elf32-bigbee"
#define TARGET_LITTLE_SYM	bee_elf32_le_vec
#define TARGET_LITTLE_NAME	"elf32-littlebee"

#define elf_info_to_howto_rel			NULL
#define elf_info_to_howto			bee_info_to_howto_rela
#define elf_backend_relocate_section		bee_elf_relocate_section
#define elf_backend_gc_mark_hook		bee_elf_gc_mark_hook
#define elf_backend_check_relocs		bee_elf_check_relocs

#define elf_backend_can_gc_sections		1
#define elf_backend_rela_normal			1

#define bfd_elf32_bfd_reloc_type_lookup		bee_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup		bee_reloc_name_lookup

#include "elf32-target.h"
