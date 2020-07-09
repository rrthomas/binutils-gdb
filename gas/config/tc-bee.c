/* tc-bee.c -- Assemble code for Bee
   Copyright (C) 2020 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* Contributed by Reuben Thomas <rrt@sc3d.org>.  */

#include "as.h"
#include "safe-ctype.h"
#define BEE_WORD_BYTES 4
#include "opcode/bee.h"
#include "elf/bee.h"

const char comment_chars[]        = "#";
const char line_separator_chars[] = ";";
const char line_comment_chars[]   = "#";

static int pending_reloc;
static htab_t opcode_hash_control;

const pseudo_typeS md_pseudo_table[] =
{
  {0, 0, 0}
};

const char FLT_CHARS[] = "rRsSfFdDxXpP";
const char EXP_CHARS[] = "eE";

static valueT md_chars_to_number (char * buf, int n);

/* Byte order.  */
extern int target_big_endian;

void
md_operand (expressionS *op __attribute__((unused)))
{
  /* Empty for now. */
}

/* This function is called once, at assembler startup time.  It sets
   up the hash table with all the opcodes in it, and also initializes
   some aliases for compatibility with other assemblers.  */

void
md_begin (void)
{
  unsigned count;
  opcode_hash_control = str_htab_create ();

  /* Insert names into hash table.  */
  const bee_opc_info_t *opcode;
  for (count = 0, opcode = bee_opc_info;
       count++ < sizeof(bee_opc_info) / sizeof(bee_opc_info[0]);
       opcode++)
    if (strcmp(opcode->name, ""))
      str_hash_insert (opcode_hash_control, opcode->name, (char *) opcode, 0);

  for (count = 0, opcode = bee_inst_info;
       count++ < sizeof(bee_inst_info) / sizeof(bee_inst_info[0]);
       opcode++)
    if (strcmp(opcode->name, ""))
      str_hash_insert (opcode_hash_control, opcode->name, (char *) opcode, 0);

  bfd_set_arch_mach (stdoutput, TARGET_ARCH, 0);
}

/* Parse an expression and then restore the input line pointer.  */

static char *
parse_exp_save_ilp (char *s, expressionS *op)
{
  char *save = input_line_pointer;

  input_line_pointer = s;
  expression (op);
  s = input_line_pointer;
  input_line_pointer = save;
  return s;
}

/* This is the guts of the machine-dependent assembler.  STR points to
   a machine dependent instruction.  This function is supposed to emit
   the frags/bytes it assembles to.  */

void
md_assemble (char *str)
{
  char *op_start;
  char *op_end;

  bee_opc_info_t *opcode;
  char *p;
  char pend;

  unsigned iword = 0;

  int nlen = 0;

  /* Drop leading whitespace.  */
  while (*str == ' ')
    str++;

  /* Find the op code end.  */
  op_start = str;
  for (op_end = str;
       *op_end && !is_end_of_line[*op_end & 0xff] && *op_end != ' ';
       op_end++)
    nlen++;

  pend = *op_end;
  *op_end = 0;

  if (nlen == 0)
    as_bad (_("can't find opcode "));
  opcode = (bee_opc_info_t *) str_hash_find (opcode_hash_control, op_start);
  *op_end = pend;

  if (opcode == NULL)
    {
      as_bad (_("unknown opcode %s"), op_start);
      return;
    }

  p = frag_more (4);

  switch (opcode->opcode)
    {
    case BEE_OP_CALLI:
    case BEE_OP_PUSHRELI:
      iword = opcode->opcode;
      while (ISSPACE (*op_end))
	op_end++;
      {
	expressionS arg;
	op_end = parse_exp_save_ilp (op_end, &arg);
	fix_new_exp (frag_now,
		     p - frag_now->fr_literal,
		     4,
		     &arg,
		     1,
		     BFD_RELOC_BEE_30_PCREL);
      }
      break;
    case BEE_OP_PUSHI:
      iword = opcode->opcode;
      while (ISSPACE (*op_end))
	op_end++;
      {
	expressionS arg;
	op_end = parse_exp_save_ilp (op_end, &arg);
	fix_new_exp (frag_now,
		     p - frag_now->fr_literal,
		     4,
		     &arg,
		     0,
		     BFD_RELOC_BEE_30);
      }
      break;
    case BEE_OP_JUMPI:
    case BEE_OP_JUMPZI:
      iword = opcode->opcode;
      while (ISSPACE (*op_end))
	op_end++;
      {
	expressionS arg;
	op_end = parse_exp_save_ilp (op_end, &arg);
	fix_new_exp (frag_now,
		     p - frag_now->fr_literal,
		     4,
		     &arg,
		     1,
		     BFD_RELOC_BEE_28_PCREL);
      }
      break;
    case BEE_OP_TRAP:
      {
        while (ISSPACE (*op_end))
          op_end++;
	expressionS arg;
        op_end = parse_exp_save_ilp (op_end, &arg);
        if (arg.X_op != O_constant)
          as_bad (_("expected constant"));
        else
          {
            int code = arg.X_add_number;
            if (code < 0 || code > BEE_MAX_TRAP)
              as_bad (_("expected trap code between 0 and 0x%x inclusive"),
                      BEE_MAX_TRAP);
            else
              iword = (code << BEE_OP2_SHIFT) | BEE_OP_TRAP;
          }
      }
      break;
    default: /* Instruction. */
      iword = opcode->opcode;
      break;
    }

  md_number_to_chars (p, iword, 4);
  dwarf2_emit_insn (4);

  while (ISSPACE (*op_end))
    op_end++;

  if (*op_end != 0)
    as_warn (_("extra stuff on line ignored"));

  if (pending_reloc)
    as_bad (_("something forgot to clean up"));
}

/* Turn a string in input_line_pointer into a floating point constant
   of type type, and store the appropriate bytes in *LITP.  The number
   of LITTLENUMS emitted is stored in *SIZEP .  An error message is
   returned, or NULL on OK.  */

const char *
md_atof (int type, char *litP, int *sizeP)
{
  int prec;
  LITTLENUM_TYPE words[4];
  char *t;
  int i;

  switch (type)
    {
    case 'f':
      prec = 2;
      break;

    case 'd':
      prec = 4;
      break;

    default:
      *sizeP = 0;
      return _("bad call to md_atof");
    }

  t = atof_ieee (input_line_pointer, type, words);
  if (t)
    input_line_pointer = t;

  *sizeP = prec * 2;

  for (i = prec - 1; i >= 0; i--)
    {
      md_number_to_chars (litP, (valueT) words[i], 2);
      litP += 2;
    }

  return NULL;
}

enum options
{
  OPTION_EB = OPTION_MD_BASE,
  OPTION_EL,
};

struct option md_longopts[] =
{
  { "EB",          no_argument, NULL, OPTION_EB},
  { "EL",          no_argument, NULL, OPTION_EL},
  { NULL,          no_argument, NULL, 0}
};

size_t md_longopts_size = sizeof (md_longopts);

const char *md_shortopts = "";

int
md_parse_option (int c ATTRIBUTE_UNUSED, const char *arg ATTRIBUTE_UNUSED)
{
  switch (c)
    {
    case OPTION_EB:
      target_big_endian = 1;
      break;
    case OPTION_EL:
      target_big_endian = 0;
      break;
    default:
      return 0;
    }

  return 1;
}

void
md_show_usage (FILE *stream ATTRIBUTE_UNUSED)
{
  fprintf (stream, _("\
  -EB                     assemble for a big endian system\n\
  -EL                     assemble for a little endian system (default)\n"));
}

/* Apply a fixup to the object file.  */

void
md_apply_fix (fixS *fixP ATTRIBUTE_UNUSED,
	      valueT * valP ATTRIBUTE_UNUSED, segT seg ATTRIBUTE_UNUSED)
{
  char *buf = fixP->fx_where + fixP->fx_frag->fr_literal;
  long val = *valP;
  long newval;
  long max, min;

  max = min = 0;
  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_32:
      if (target_big_endian)
	{
	  buf[0] = val >> 24;
	  buf[1] = val >> 16;
	  buf[2] = val >> 8;
	  buf[3] = val >> 0;
	}
      else
	{
	  buf[3] = val >> 24;
	  buf[2] = val >> 16;
	  buf[1] = val >> 8;
	  buf[0] = val >> 0;
	}
      buf += 4;
      break;

    case BFD_RELOC_16:
      if (target_big_endian)
	{
	  buf[0] = val >> 8;
	  buf[1] = val >> 0;
	}
      else
	{
	  buf[1] = val >> 8;
	  buf[0] = val >> 0;
	}
      buf += 2;
      break;

    case BFD_RELOC_8:
      *buf++ = val;
      break;

    case BFD_RELOC_BEE_30:
      if (!val)
	break;
      newval = md_chars_to_number (buf, 4);
      newval |= val << 2;
      md_number_to_chars (buf, newval, 4);
      break;

    case BFD_RELOC_BEE_30_PCREL:
      if (!val)
	break;
      if (val & 3)
	as_bad_where (fixP->fx_file, fixP->fx_line,
                      _("pcrel not aligned BFD_RELOC_BEE_30PCREL"));
      newval = md_chars_to_number (buf, 4);
      newval |= val;
      md_number_to_chars (buf, newval, 4);
      break;

    case BFD_RELOC_BEE_28_PCREL:
      if (!val)
	break;
      if (val & 3)
	as_bad_where (fixP->fx_file, fixP->fx_line,
                      _("pcrel not aligned BFD_RELOC_BEE_28PCREL"));
      newval = md_chars_to_number (buf, 4);
      newval |= val << 2;
      md_number_to_chars (buf, newval, 4);
      break;

    default:
      abort ();
    }

  if (max != 0 && (val < min || val > max))
    as_bad_where (fixP->fx_file, fixP->fx_line, _("offset out of range"));

  if (fixP->fx_addsy == NULL && fixP->fx_pcrel == 0)
    fixP->fx_done = 1;
}

/* Put number into target byte order.  */

void
md_number_to_chars (char * ptr, valueT use, int nbytes)
{
  if (target_big_endian)
    number_to_chars_bigendian (ptr, use, nbytes);
  else
    number_to_chars_littleendian (ptr, use, nbytes);
}

/* Convert from target byte order to host byte order.  */

static valueT
md_chars_to_number (char * buf, int n)
{
  valueT result = 0;
  unsigned char * where = (unsigned char *) buf;

  if (target_big_endian)
    {
      while (n--)
	{
	  result <<= 8;
	  result |= (*where++ & 255);
	}
    }
  else
    {
      while (n--)
	{
	  result <<= 8;
	  result |= (where[n] & 255);
	}
    }

  return result;
}

/* Generate a machine-dependent relocation.  */
arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixP)
{
  arelent *relP;
  bfd_reloc_code_real_type code;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_32:
    case BFD_RELOC_8:
    case BFD_RELOC_BEE_30_PCREL:
    case BFD_RELOC_BEE_30:
      code = fixP->fx_r_type;
      break;
    default:
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("Semantics error.  This type of operand can not be relocated, it must be an assembly-time constant"));
      return 0;
    }

  relP = XNEW (arelent);
  relP->sym_ptr_ptr = XNEW (asymbol *);
  *relP->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);
  relP->address = fixP->fx_frag->fr_address + fixP->fx_where;

  relP->addend = fixP->fx_offset;

  /* This is the standard place for KLUDGEs to work around bugs in
     bfd_install_relocation (first such note in the documentation
     appears with binutils-2.8).

     That function bfd_install_relocation does the wrong thing with
     putting stuff into the addend of a reloc (it should stay out) for a
     weak symbol.  The really bad thing is that it adds the
     "segment-relative offset" of the symbol into the reloc.  In this
     case, the reloc should instead be relative to the symbol with no
     other offset than the assembly code shows; and since the symbol is
     weak, any local definition should be ignored until link time (or
     thereafter).
     To wit:  weaksym+42  should be weaksym+42 in the reloc,
     not weaksym+(offset_from_segment_of_local_weaksym_definition)

     To "work around" this, we subtract the segment-relative offset of
     "known" weak symbols.  This evens out the extra offset.

     That happens for a.out but not for ELF, since for ELF,
     bfd_install_relocation uses the "special function" field of the
     howto, and does not execute the code that needs to be undone.  */

  if (OUTPUT_FLAVOR == bfd_target_aout_flavour
      && fixP->fx_addsy && S_IS_WEAK (fixP->fx_addsy)
      && ! bfd_is_und_section (S_GET_SEGMENT (fixP->fx_addsy)))
    {
      relP->addend -= S_GET_VALUE (fixP->fx_addsy);
    }

  relP->howto = bfd_reloc_type_lookup (stdoutput, code);
  if (! relP->howto)
    {
      const char *name;

      name = S_GET_NAME (fixP->fx_addsy);
      if (name == NULL)
	name = _("<unknown>");
      as_fatal (_("Cannot generate relocation type for symbol %s, code %s"),
		name, bfd_get_reloc_code_name (code));
    }

  return relP;
}

/* Decide from what point a pc-relative relocation is relative to,
   relative to the pc-relative fixup.  Er, relatively speaking.  */
long
md_pcrel_from (fixS *fixP)
{
  valueT addr = fixP->fx_where + fixP->fx_frag->fr_address;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_32:
    case BFD_RELOC_8:
    case BFD_RELOC_BEE_30_PCREL:
    case BFD_RELOC_BEE_28_PCREL:
      return addr;
    default:
      abort ();
      return addr;
    }
}
