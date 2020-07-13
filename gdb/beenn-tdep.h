/* Target-dependent code for Bee.

   Copyright (C) 2009-2022 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "value.h"
#include "inferior.h"
#include "symfile.h"
#include "objfiles.h"
#include "osabi.h"
#include "language.h"
#include "arch-utils.h"
#include "regcache.h"
#include "trad-frame.h"
#include "dis-asm.h"
#include "record.h"
#include "record-full.h"

#include "opcode/bee.h"
#include "bee-tdep.h"
#include <algorithm>

struct bee_unwind_cache
{
  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR prev_sp;
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;
  int size;
  /* How far SP has been offset from the start of the stack frame (as
     defined by the previous frame's stack pointer).  */
  LONGEST sp_offset;
  int uses_frame;
  /* Table indicating the location of each and every register.  */
  struct trad_frame_saved_reg *saved_regs;
};

/* Read an unsigned integer from the inferior, and adjust
   endianness.  */
static ULONGEST
bee_process_readu (CORE_ADDR addr, gdb_byte *buf,
		     int length, enum bfd_endian byte_order)
{
  if (target_read_memory (addr, buf, length))
    {
      if (record_debug)
	printf_unfiltered (_("Process record: error reading memory at "
			     "addr 0x%s len = %d.\n"),
			   paddress (target_gdbarch (), addr), length);
      return -1;
    }

  return extract_unsigned_integer (buf, length, byte_order);
}

/* Read a stack item from the inferior, and adjust endianness.  */
static ULONGEST
bee_stack_readu (CORE_ADDR s0, CORE_ADDR sp, unsigned pos, gdb_byte *buf,
		   enum bfd_endian byte_order)
{
  return
    bee_process_readu (s0 + (sp - (pos + 1)) * BEE_WORD_BYTES, buf, BEE_WORD_BYTES, byte_order);
}

/* Insert a single step breakpoint.  */

std::vector<CORE_ADDR>
beeNN_software_single_step (struct regcache *regcache);

std::vector<CORE_ADDR>
beeNN_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR addr;
  gdb_byte buf[BEE_WORD_BYTES];
  intptr_t inst;
  uintptr_t dest;
  ULONGEST s0, sp;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  std::vector<CORE_ADDR> next_pcs;

  addr = regcache_read_pc (regcache);
  inst = (int64_t) bee_process_readu (addr, buf, BEE_WORD_BYTES, byte_order);

  /* Decode instruction.  */
  switch (inst & BEE_OP1_MASK)
    {
    case BEE_OP_CALLI:
      next_pcs.push_back (addr + (inst >> BEE_OP1_SHIFT) * BEE_WORD_BYTES);
      break;
    case BEE_OP_PUSHI:
    case BEE_OP_PUSHRELI:
      next_pcs.push_back (addr + BEE_WORD_BYTES);
      break;
    default:
      switch (inst & BEE_OP2_MASK)
	{
	case BEE_OP_JUMPI:
	case BEE_OP_JUMPZI:
	  next_pcs.push_back (addr + (inst >> BEE_OP2_SHIFT) * BEE_WORD_BYTES);
	  break;
	case BEE_OP_TRAP:
	  next_pcs.push_back (addr + BEE_WORD_BYTES);
	  break;
	case BEE_OP_INSN:
	  switch (inst >> BEE_OP2_SHIFT)
	    {
	    case BEE_INSN_NOP:
	    case BEE_INSN_NOT:
	    case BEE_INSN_AND:
	    case BEE_INSN_OR:
	    case BEE_INSN_XOR:
	    case BEE_INSN_LSHIFT:
	    case BEE_INSN_RSHIFT:
	    case BEE_INSN_ARSHIFT:
	    case BEE_INSN_POP:
	    case BEE_INSN_DUP:
	    case BEE_INSN_SET:
	    case BEE_INSN_SWAP:
	    case BEE_INSN_LOAD:
	    case BEE_INSN_STORE:
	    case BEE_INSN_LOAD1:
	    case BEE_INSN_STORE1:
	    case BEE_INSN_LOAD2:
	    case BEE_INSN_STORE2:
	    case BEE_INSN_LOAD4:
	    case BEE_INSN_STORE4:
	    case BEE_INSN_LOAD_IA:
	    case BEE_INSN_STORE_DB:
	    case BEE_INSN_LOAD_IB:
	    case BEE_INSN_STORE_DA:
	    case BEE_INSN_LOAD_DA:
	    case BEE_INSN_STORE_IB:
	    case BEE_INSN_LOAD_DB:
	    case BEE_INSN_STORE_IA:
	    case BEE_INSN_NEG:
	    case BEE_INSN_ADD:
	    case BEE_INSN_MUL:
	    case BEE_INSN_DIVMOD:
	    case BEE_INSN_UDIVMOD:
	    case BEE_INSN_EQ:
	    case BEE_INSN_LT:
	    case BEE_INSN_ULT:
	    case BEE_INSN_PUSHS:
	    case BEE_INSN_POPS:
	    case BEE_INSN_DUPS:
	    case BEE_INSN_WORD_BYTES:
	    case BEE_INSN_GET_SSIZE:
	    case BEE_INSN_GET_SP:
	    case BEE_INSN_SET_SP:
	    case BEE_INSN_GET_DSIZE:
	    case BEE_INSN_GET_DP:
	    case BEE_INSN_SET_DP:
	    case BEE_INSN_GET_HANDLER_SP:
	      next_pcs.push_back (addr + BEE_WORD_BYTES);
	      break;
	    case BEE_INSN_JUMP:
	    case BEE_INSN_CALL:
	    case BEE_INSN_CATCH:
	      regcache_cooked_read_unsigned (regcache, bee_dp_regnum, &sp);
	      if (sp > 0)
		{
		  regcache_cooked_read_unsigned (regcache, bee_d0_regnum, &s0);
		  dest = (intptr_t) bee_stack_readu (s0, sp, 0, buf, byte_order);
		  next_pcs.push_back (dest);
		}
	      break;
	    case BEE_INSN_JUMPZ:
	      regcache_cooked_read_unsigned (regcache, bee_dp_regnum, &sp);
	      if (sp > 1)
		{
		  regcache_cooked_read_unsigned (regcache, bee_d0_regnum, &s0);
		  dest = (intptr_t) bee_stack_readu (s0, sp, 1, buf, byte_order);
		  next_pcs.push_back (dest);
		  next_pcs.push_back (addr + BEE_WORD_BYTES);
		}
	      break;
	    case BEE_INSN_RET:
	      regcache_cooked_read_unsigned (regcache, bee_sp_regnum, &sp);
	      if (sp > 0)
		{
		  regcache_cooked_read_unsigned (regcache, bee_s0_regnum, &s0);
		  dest = (intptr_t) bee_stack_readu (s0, sp, 0, buf, byte_order);
		  next_pcs.push_back (dest);
		}
	      break;
	    case BEE_INSN_THROW:
	      regcache_cooked_read_unsigned (regcache, bee_handler_sp_regnum, &sp);
	      if (sp > 1)
		{
		  regcache_cooked_read_unsigned (regcache, bee_s0_regnum, &s0);
		  dest = (intptr_t) bee_stack_readu (s0, sp, 0, buf, byte_order);
		  next_pcs.push_back (dest);
		}
	      break;
	    case BEE_INSN_BREAK:
	    default:
	      break;
	    }
	}
      break;
    }

  return next_pcs;
}

/* Parse the current instruction and record the values of the registers and
   memory that will be changed in current instruction to "record_arch_list".
   Return -1 if something wrong.  */

#define record_stack_item(s0, sp, depth)				\
  record_full_arch_list_add_mem (s0 + (sp - (depth + 1)) * BEE_WORD_BYTES, \
				 BEE_WORD_BYTES)

static inline int
_record_stack(struct regcache *regcache,
	      int sp_regnum, ULONGEST s0, ULONGEST sp, ULONGEST ssize,
	      int pops, int pushes)
{
  if (sp < pops || sp - pops + pushes > ssize
      || record_full_arch_list_add_reg (regcache, sp_regnum))
    return -1;
  if (pushes > 0
      && record_full_arch_list_add_mem (s0 + (sp - pops) * BEE_WORD_BYTES,
					pushes * BEE_WORD_BYTES))
    return -1;
  return 0;
}

#define record_stack(pops, pushes)				\
  _record_stack (regcache, bee_sp_regnum, s0, sp, ssize, pops, pushes)
#define record_data_stack(pops, pushes)				\
  _record_stack (regcache, bee_dp_regnum, d0, dp, dsize, pops, pushes)

#define record_load(size)				\
  (record_data_stack (1, 1)				\
   || addr % size != 0)
#define record_store(size)				\
  (record_data_stack (2, 0)				\
   || addr % size != 0					\
   || record_full_arch_list_add_mem (addr, size))

#define record_load_incdec(size)			\
  (record_data_stack (1, 2)				\
   || addr % size != 0)
#define record_store_incdec(size)			\
  (record_data_stack (2, 1)				\
   || addr % size != 0					\
   || record_full_arch_list_add_mem (addr, size))


int
beeNN_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		      CORE_ADDR addr);

int
beeNN_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		      CORE_ADDR iaddr)
{
  gdb_byte buf[BEE_WORD_BYTES];
  LONGEST ir;
  ULONGEST s0, ssize, sp, d0, dsize, dp, handler_sp;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  regcache_cooked_read_unsigned (regcache, bee_s0_regnum, &s0);
  regcache_cooked_read_unsigned (regcache, bee_ssize_regnum, &ssize);
  regcache_cooked_read_unsigned (regcache, bee_sp_regnum, &sp);
  regcache_cooked_read_unsigned (regcache, bee_d0_regnum, &d0);
  regcache_cooked_read_unsigned (regcache, bee_dsize_regnum, &dsize);
  regcache_cooked_read_unsigned (regcache, bee_dp_regnum, &dp);
  regcache_cooked_read_unsigned (regcache, bee_handler_sp_regnum, &handler_sp);

  if (record_debug > 1)
    gdb_printf (gdb_stdlog, "Process record: bee_process_record "
		"addr = 0x%s\n",
		paddress (target_gdbarch (), iaddr));

  ir = (LONGEST) bee_process_readu (iaddr, buf, BEE_WORD_BYTES, byte_order);

  /* Decode instruction.  */
  switch (ir & BEE_OP1_MASK) {
  case BEE_OP_CALLI:
    if (record_stack (0, 1))
      return -1;
    break;
  case BEE_OP_PUSHI:
  case BEE_OP_PUSHRELI:
    if (record_data_stack (0, 1))
      return -1;
    break;
  default:
    switch (ir & BEE_OP2_MASK) {
    case BEE_OP_JUMPI:
      /* Do nothing.  */
      break;
    case BEE_OP_JUMPZI:
      if (record_data_stack (1, 0))
	return -1;
      break;
    case BEE_OP_TRAP:
      {
	ULONGEST lib = (ULONGEST)ir >> BEE_OP2_SHIFT;
	ULONGEST function;

	if (lib != BEE_TRAP_LIBC
	    || record_data_stack (1, 0))
	  return -1;
	function = bee_stack_readu (d0, dp, 0, buf, byte_order);
	switch (function) {
	case BEE_TRAP_LIBC_STRLEN: // ( a-addr -- u )
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_STRNCPY: // ( a-addr1 a-addr2 u -- a-addr3 )
	  if (record_data_stack (3, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_STDIN:
	case BEE_TRAP_LIBC_STDOUT:
	case BEE_TRAP_LIBC_STDERR:
	case BEE_TRAP_LIBC_O_RDONLY:
	case BEE_TRAP_LIBC_O_WRONLY:
	case BEE_TRAP_LIBC_O_RDWR:
	case BEE_TRAP_LIBC_O_CREAT:
	case BEE_TRAP_LIBC_O_TRUNC:
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_OPEN: // ( c-addr flags -- fd )
	  if (record_data_stack (2, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_CLOSE:
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_READ:
	case BEE_TRAP_LIBC_WRITE:
	  if (record_data_stack (3, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_SEEK_SET:
	case BEE_TRAP_LIBC_SEEK_CUR:
	case BEE_TRAP_LIBC_SEEK_END:
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_LSEEK:
	  if (record_data_stack (4, 2))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_FDATASYNC:
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_RENAME:
	  if (record_data_stack (2, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_REMOVE:
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_FILE_SIZE:
	  if (record_data_stack (1, 3))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_RESIZE_FILE:
	  if (record_data_stack (3, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_FILE_STATUS:
	  if (record_data_stack (1, 2))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_ARGC: // ( -- u )
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	case BEE_TRAP_LIBC_ARGV: // ( -- a-addr )
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	default:
	  /* Do nothing.  */
	  break;
	}
      }
      break;
    case BEE_OP_INSN:
      {
	ULONGEST opcode = (ULONGEST)ir >> BEE_OP2_SHIFT;
	switch (opcode) {
	case BEE_INSN_NOP:
	  /* Do nothing.  */
	  break;
	case BEE_INSN_NOT:
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_INSN_AND:
	case BEE_INSN_OR:
	case BEE_INSN_XOR:
	case BEE_INSN_LSHIFT:
	case BEE_INSN_RSHIFT:
	case BEE_INSN_ARSHIFT:
	  if (record_data_stack (2, 1))
	    return -1;
	  break;
	case BEE_INSN_POP:
	  if (record_data_stack (1, 0))
	    return -1;
	  break;
	case BEE_INSN_DUP:
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_INSN_SET:
	  {
	    ULONGEST depth = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_data_stack (2, 0)
		|| record_stack_item (d0, dp, depth))
	      return -1;
	  }
	  break;
	case BEE_INSN_SWAP:
	  {
	    ULONGEST depth = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_data_stack (1, 0)
		|| record_stack_item (d0, dp, 0)
		|| record_stack_item (d0, dp, depth))
	      return -1;
	  }
	  break;
	case BEE_INSN_JUMP:
	  if (record_data_stack (1, 0))
	    return -1;
	  break;
	case BEE_INSN_JUMPZ:
	  if (record_data_stack (2, 0))
	    return -1;
	  break;
	case BEE_INSN_CALL:
	  if (record_data_stack (1, 0))
	    return -1;
	  if (record_stack (0, 1))
	    return -1;
	  break;
	case BEE_INSN_RET:
	  if (record_stack (1, 0))
	    return -1;
	  if (sp < handler_sp) {
	    if (record_stack (1, 0) || record_data_stack (0, 1))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_load (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_store (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD1:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_load (1))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE1:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_store (1))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD2:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_load (2))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE2:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_store (2))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD4:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_load (4))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE4:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_store (4))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD_IA:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_load_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE_DB:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order) - BEE_WORD_BYTES;

	    if (record_store_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD_IB:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order) + BEE_WORD_BYTES;

	    if (record_load_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE_DA:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_store_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD_DA:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_load_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE_IB:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order) + BEE_WORD_BYTES;

	    if (record_store_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_LOAD_DB:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order) - BEE_WORD_BYTES;

	    if (record_load_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_STORE_IA:
	  {
	    ULONGEST addr = bee_stack_readu (d0, dp, 0, buf, byte_order);

	    if (record_store_incdec (BEE_WORD_BYTES))
	      return -1;
	  }
	  break;
	case BEE_INSN_NEG:
	  if (record_data_stack (1, 1))
	    return -1;
	  break;
	case BEE_INSN_ADD:
	case BEE_INSN_MUL:
	  if (record_data_stack (2, 1))
	    return -1;
	  break;
	case BEE_INSN_DIVMOD:
	case BEE_INSN_UDIVMOD:
	  if (record_data_stack (2, 2))
	    return -1;
	  break;
	case BEE_INSN_EQ:
	case BEE_INSN_LT:
	case BEE_INSN_ULT:
	  if (record_data_stack (2, 1))
	    return -1;
	  break;
	case BEE_INSN_PUSHS:
	  if (record_data_stack (1, 0))
	    return -1;
	  if (record_stack (0, 1))
	    return -1;
	  break;
	case BEE_INSN_POPS:
	  if (record_stack (1, 0))
	    return -1;
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	case BEE_INSN_DUPS:
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	case BEE_INSN_CATCH:
	  if (record_data_stack (1, 0)
	      || record_stack (0, 2)
	      || record_full_arch_list_add_reg (regcache, bee_handler_sp_regnum))
	    return -1;
	  break;
	case BEE_INSN_THROW:
	  {
	    if (handler_sp == 0)
	      return -1;
	    if (record_data_stack (0, 1)
		|| record_stack (2, 0)
		|| record_full_arch_list_add_reg (regcache, bee_sp_regnum)
		|| record_full_arch_list_add_reg (regcache, bee_handler_sp_regnum))
	      return -1;
	  }
	  break;
	case BEE_INSN_BREAK:
	  /* Do nothing.  */
	  break;
	case BEE_INSN_WORD_BYTES:
	case BEE_INSN_GET_SSIZE:
	case BEE_INSN_GET_SP:
	case BEE_INSN_GET_DSIZE:
	case BEE_INSN_GET_DP:
	case BEE_INSN_GET_HANDLER_SP:
	  if (record_data_stack (0, 1))
	    return -1;
	  break;
	case BEE_INSN_SET_SP:
	case BEE_INSN_SET_DP:
	  if (record_data_stack (1, 0))
	    return -1;
	  break;
	default:
	  /* Do nothing.  */
	  break;
	}
      }
      break;
    }
  }

  if (record_full_arch_list_add_reg (regcache, bee_pc_regnum)
      || record_full_arch_list_add_end ())
    return -1;
  return 0;
}
