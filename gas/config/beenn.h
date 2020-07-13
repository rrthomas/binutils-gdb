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
#include "opcode/bee.h"
#include "elf/bee.h"

static int pending_reloc;
static htab_t opcode_hash_control;

/* Table to identify an instruction from its 4 LSbits.  */
extern const bee_opc_info_t BEENN_(_opc_info)[16];
/* Table of information about short instructions (BEE_INSN_*).  */
extern const bee_opc_info_t BEENN_(_inst_info)[0x40];

/* This function is called once, at assembler startup time.  It sets
   up the hash table with all the opcodes in it.  */

void _BEENN(md_begin_) (void);

void
_BEENN(md_begin_) (void)
{
  unsigned count;
  opcode_hash_control = str_htab_create ();

  /* Insert names into hash table.  */
  const bee_opc_info_t *opcode;
  for (count = 0, opcode = BEENN_(_opc_info);
       count++ < sizeof(BEENN_(_opc_info)) / sizeof(BEENN_(_opc_info)[0]);
       opcode++)
    if (strcmp(opcode->name, ""))
      str_hash_insert (opcode_hash_control, opcode->name, (char *) opcode, 0);

  for (count = 0, opcode = BEENN_(_inst_info);
       count++ < sizeof(BEENN_(_inst_info)) / sizeof(BEENN_(_inst_info[0]));
       opcode++)
    if (strcmp(opcode->name, ""))
      str_hash_insert (opcode_hash_control, opcode->name, (char *) opcode, 0);
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

void _BEENN(md_assemble_) (char *str);

void
_BEENN(md_assemble_) (char *str)
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

  p = frag_more (BEE_WORD_BYTES);

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
		     BEE_WORD_BYTES,
		     &arg,
		     1,
		     BEE_RELOC_PCREL_LONG);
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
		     BEE_WORD_BYTES,
		     &arg,
		     0,
		     BEE_RELOC_LONG);
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
		     BEE_WORD_BYTES,
		     &arg,
		     1,
		     BEE_RELOC_PCREL_SHORT);
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
            offsetT code = arg.X_add_number;
            if (code < 0 || code > BEE_MAX_TRAP)
              as_bad (_("expected trap code between 0 and 0x%lx inclusive"),
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

  md_number_to_chars (p, iword, BEE_WORD_BYTES);
  dwarf2_emit_insn (BEE_WORD_BYTES);

  while (ISSPACE (*op_end))
    op_end++;

  if (*op_end != 0)
    as_warn (_("extra stuff on line ignored"));

  if (pending_reloc)
    as_bad (_("something forgot to clean up"));
}
