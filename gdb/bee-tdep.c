/* Target-dependent code for Bee.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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

#include "bee-tdep.h"

/* Bee register names.  */

#define xstr(s) #s
#define str(s) xstr(s)
#define R(reg, type) str(reg),
static const char *bee_register_names[] = {
#include "opcodes/bee-regs.h"
};
#undef R
#undef str
#undef xstr

/* Use an invalid address value as 'not available' marker.  */
enum { REG_UNAVAIL = (CORE_ADDR) -1 };

struct bee_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR pc;
  LONGEST framesize;
  CORE_ADDR saved_regs[bee_num_regs];
  CORE_ADDR saved_sp;
};

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
bee_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align to the size of an instruction (so that they can safely be
     pushed onto the stack.  */
  return sp & -8;
}

/* Implement the "register_name" gdbarch method.  */

static const char *
bee_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr < 0 || reg_nr >= bee_num_regs)
    return NULL;
  return bee_register_names[reg_nr];
}

/* Implement the "register_type" gdbarch method.  */

static struct type *
bee_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr == bee_pc_regnum)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else if (reg_nr == bee_m0_regnum || reg_nr == bee_s0_regnum || reg_nr == bee_d0_regnum)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint64;
}

constexpr gdb_byte bee32_break_insn[] = { 0x5f, 0x02, 0x00, 0x00 };

typedef BP_MANIPULATION (bee32_break_insn) bee32_breakpoint;

constexpr gdb_byte bee64_break_insn[] = { 0x2f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

typedef BP_MANIPULATION (bee64_break_insn) bee64_breakpoint;

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

/* static void */
/* bee_store_return_value (struct type *type, struct regcache *regcache, */
/* 			 const gdb_byte *valbuf) */
/* { */
/*   struct gdbarch *gdbarch = regcache->arch (); */
/*   enum bfd_endian byte_order = gdbarch_byte_order (gdbarch); */
/*   CORE_ADDR regval; */
/*   int len = TYPE_LENGTH (type); */

/*   /\* Things always get returned in RET1_REGNUM, RET2_REGNUM.  *\/ */
/*   regval = extract_unsigned_integer (valbuf, len > 4 ? 4 : len, byte_order); */
/*   regcache_cooked_write_unsigned (regcache, RET1_REGNUM, regval); */
/*   if (len > 4) */
/*     { */
/*       regval = extract_unsigned_integer (valbuf + 4, len - 4, byte_order); */
/*       regcache_cooked_write_unsigned (regcache, RET1_REGNUM + 1, regval); */
/*     } */
/* } */

/* /\* Decode the instructions within the given address range.  Decide */
/*    when we must have reached the end of the function prologue.  If a */
/*    frame_info pointer is provided, fill in its saved_regs etc. */

/*    Returns the address of the first instruction after the prologue.  *\/ */

/* static CORE_ADDR */
/* bee_analyze_prologue (CORE_ADDR start_addr, CORE_ADDR end_addr, */
/* 			struct bee_frame_cache *cache, */
/* 			struct gdbarch *gdbarch) */
/* { */
/*   enum bfd_endian byte_order = gdbarch_byte_order (gdbarch); */
/*   CORE_ADDR next_addr; */
/*   ULONGEST inst, inst2; */
/*   LONGEST offset; */
/*   int regnum; */

/*   /\* Record where the jsra instruction saves the PC and FP.  *\/ */
/*   cache->saved_regs[bee_pc_regnum] = -4; */
/*   cache->saved_regs[bee_fp_regnum] = 0; */
/*   cache->framesize = 0; */

/*   if (start_addr >= end_addr) */
/*     return end_addr; */

/*   for (next_addr = start_addr; next_addr < end_addr; ) */
/*     { */
/*       inst = read_memory_unsigned_integer (next_addr, 2, byte_order); */

/*       /\* Match "push $sp $rN" where N is between 0 and 13 inclusive.  *\/ */
/*       if (inst >= 0x0612 && inst <= 0x061f) */
/* 	{ */
/* 	  regnum = inst & 0x000f; */
/* 	  cache->framesize += 4; */
/* 	  cache->saved_regs[regnum] = cache->framesize; */
/* 	  next_addr += 2; */
/* 	} */
/*       else */
/* 	break; */
/*     } */

/*   inst = read_memory_unsigned_integer (next_addr, 2, byte_order); */

/*   /\* Optional stack allocation for args and local vars <= 4 */
/*      byte.  *\/ */
/*   if (inst == 0x01e0)          /\* ldi.l $r12, X *\/ */
/*     { */
/*       offset = read_memory_integer (next_addr + 2, 4, byte_order); */
/*       inst2 = read_memory_unsigned_integer (next_addr + 6, 2, byte_order); */
      
/*       if (inst2 == 0x291e)     /\* sub.l $sp, $r12 *\/ */
/* 	{ */
/* 	  cache->framesize += offset; */
/* 	} */
      
/*       return (next_addr + 8); */
/*     } */
/*   else if ((inst & 0xff00) == 0x9100)   /\* dec $sp, X *\/ */
/*     { */
/*       cache->framesize += (inst & 0x00ff); */
/*       next_addr += 2; */

/*       while (next_addr < end_addr) */
/* 	{ */
/* 	  inst = read_memory_unsigned_integer (next_addr, 2, byte_order); */
/* 	  if ((inst & 0xff00) != 0x9100) /\* no more dec $sp, X *\/ */
/* 	    break; */
/* 	  cache->framesize += (inst & 0x00ff); */
/* 	  next_addr += 2; */
/* 	} */
/*     } */

/*   return next_addr; */
/* } */

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
bee_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  return RETURN_VALUE_STRUCT_CONVENTION;
}

/* Find the end of function prologue.  */

static CORE_ADDR
bee_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* CORE_ADDR func_addr = 0, func_end = 0; */
  /* const char *func_name; */

  /* /\* See if we can determine the end of the prologue via the symbol table. */
  /*    If so, then return either PC, or the PC after the prologue, whichever */
  /*    is greater.  *\/ */
  /* if (find_pc_partial_function (pc, &func_name, &func_addr, &func_end)) */
  /*   { */
  /*     CORE_ADDR post_prologue_pc */
  /* 	= skip_prologue_using_sal (gdbarch, func_addr); */
  /*     if (post_prologue_pc != 0) */
  /* 	return std::max (pc, post_prologue_pc); */
  /*     else */
  /* 	{ */
  /* 	  /\* Can't determine prologue from the symbol table, need to examine */
  /* 	     instructions.  *\/ */
  /* 	  struct symtab_and_line sal; */
  /* 	  struct symbol *sym; */
  /* 	  struct bee_frame_cache cache; */
  /* 	  CORE_ADDR plg_end; */
	  
  /* 	  memset (&cache, 0, sizeof cache); */
	  
  /* 	  plg_end = bee_analyze_prologue (func_addr, */
  /* 					    func_end, &cache, gdbarch); */
  /* 	  /\* Found a function.  *\/ */
  /* 	  sym = lookup_symbol (func_name, NULL, VAR_DOMAIN, NULL).symbol; */
  /* 	  /\* Don't use line number debug info for assembly source */
  /* 	     files.  *\/ */
  /* 	  if (sym && sym->language () != language_asm) */
  /* 	    { */
  /* 	      sal = find_pc_line (func_addr, 0); */
  /* 	      if (sal.end && sal.end < func_end) */
  /* 		{ */
  /* 		  /\* Found a line number, use it as end of */
  /* 		     prologue.  *\/ */
  /* 		  return sal.end; */
  /* 		} */
  /* 	    } */
  /* 	  /\* No useable line symbol.  Use result of prologue parsing */
  /* 	     method.  *\/ */
  /* 	  return plg_end; */
  /* 	} */
  /*   } */

  /* No function symbol -- just return the PC.  */
  return (CORE_ADDR) pc;
}

/* Allocate and initialize a bee_frame_cache object.  */

static struct bee_frame_cache *
bee_alloc_frame_cache (void)
{
  struct bee_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct bee_frame_cache);

  cache->base = 0;
  cache->saved_sp = 0;
  cache->pc = 0;
  cache->framesize = 0;
  for (i = 0; i < bee_num_regs; ++i)
    cache->saved_regs[i] = REG_UNAVAIL;

  return cache;
}

/* Populate a bee_frame_cache object for this_frame.  */

static struct bee_frame_cache *
bee_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  struct bee_frame_cache *cache;
  /* CORE_ADDR current_pc; */
  /* int i; */

  if (*this_cache)
    return (struct bee_frame_cache *) *this_cache;

  cache = bee_alloc_frame_cache ();
  *this_cache = cache;

  /* cache->base = get_frame_register_unsigned (this_frame, bee_fp_regnum); */
  /* if (cache->base == 0) */
  /*   return cache; */

  /* cache->pc = get_frame_func (this_frame); */
  /* current_pc = get_frame_pc (this_frame); */
  /* if (cache->pc) */
  /*   { */
  /*     struct gdbarch *gdbarch = get_frame_arch (this_frame); */
  /*     bee_analyze_prologue (cache->pc, current_pc, cache, gdbarch); */
  /*   } */

  /* cache->saved_sp = cache->base - cache->framesize; */

  /* for (i = 0; i < bee_num_regs; ++i) */
  /*   if (cache->saved_regs[i] != REG_UNAVAIL) */
  /*     cache->saved_regs[i] = cache->base - cache->saved_regs[i]; */

  return cache;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
bee_frame_this_id (struct frame_info *this_frame,
		    void **this_prologue_cache, struct frame_id *this_id)
{
  struct bee_frame_cache *cache = bee_frame_cache (this_frame,
						   this_prologue_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->saved_sp, cache->pc);
}

/* Get the value of register regnum in the previous stack frame.  */

static struct value *
bee_frame_prev_register (struct frame_info *this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct bee_frame_cache *cache = bee_frame_cache (this_frame,
						   this_prologue_cache);

  gdb_assert (regnum >= 0);

  if (regnum == bee_sp_regnum && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < bee_num_regs && cache->saved_regs[regnum] != REG_UNAVAIL)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind bee_frame_unwind = {
  "bee prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  bee_frame_this_id,
  bee_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Return the base address of this_frame.  */

static CORE_ADDR
bee_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct bee_frame_cache *cache = bee_frame_cache (this_frame,
						       this_cache);

  return cache->base;
}

static const struct frame_base bee_frame_base = {
  &bee_frame_unwind,
  bee_frame_base_address,
  bee_frame_base_address,
  bee_frame_base_address
};

std::vector<CORE_ADDR>
bee32_software_single_step (struct regcache *regcache);
std::vector<CORE_ADDR>
bee64_software_single_step (struct regcache *regcache);

static std::vector<CORE_ADDR>
bee_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const struct bfd_arch_info *info = gdbarch_bfd_arch_info (gdbarch);
  if (info->bits_per_word == 32)
    return bee32_software_single_step (regcache);
  else
    return bee64_software_single_step (regcache);
}

int
bee32_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		    CORE_ADDR addr);
int
bee64_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		    CORE_ADDR addr);

static int
bee_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		    CORE_ADDR addr)
{
  const struct bfd_arch_info *info = gdbarch_bfd_arch_info (gdbarch);
  if (info->bits_per_word == 32)
    return bee32_process_record (gdbarch, regcache, addr);
  else
    return bee64_process_record (gdbarch, regcache, addr);
}

/* Allocate and initialize the bee gdbarch object.  */

static struct gdbarch *
bee_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* Allocate space for the new architecture.  */
  tdep = XCNEW (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  set_gdbarch_wchar_bit (gdbarch, 32);
  set_gdbarch_wchar_signed (gdbarch, 0);

  if (info.bfd_arch_info)
    {
      set_gdbarch_long_bit (gdbarch, info.bfd_arch_info->bits_per_word);
      set_gdbarch_ptr_bit (gdbarch, info.bfd_arch_info->bits_per_word);
    }

  set_gdbarch_num_regs (gdbarch, bee_num_regs);
  set_gdbarch_sp_regnum (gdbarch, bee_sp_regnum);
  set_gdbarch_pc_regnum (gdbarch, bee_pc_regnum);
  set_gdbarch_register_name (gdbarch, bee_register_name);
  set_gdbarch_register_type (gdbarch, bee_register_type);

  set_gdbarch_return_value (gdbarch, bee_return_value);

  set_gdbarch_skip_prologue (gdbarch, bee_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  if (info.bfd_arch_info && info.bfd_arch_info->bits_per_word == 64)
    {
      set_gdbarch_breakpoint_kind_from_pc (gdbarch,
					   bee64_breakpoint::kind_from_pc);
      set_gdbarch_sw_breakpoint_from_kind (gdbarch,
					   bee64_breakpoint::bp_from_kind);
    }
  else
    {
      set_gdbarch_breakpoint_kind_from_pc (gdbarch,
					   bee32_breakpoint::kind_from_pc);
      set_gdbarch_sw_breakpoint_from_kind (gdbarch,
					   bee32_breakpoint::bp_from_kind);
    }
  set_gdbarch_frame_align (gdbarch, bee_frame_align);

  frame_base_set_default (gdbarch, &bee_frame_base);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Hook in the default unwinders.  */
  frame_unwind_append_unwinder (gdbarch, &bee_frame_unwind);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, bee_software_single_step);

  /* Support simple overlay manager.  */
  set_gdbarch_overlay_update (gdbarch, simple_overlay_update);

  /* Support reverse debugging.  */
  set_gdbarch_process_record (gdbarch, bee_process_record);

  return gdbarch;
}

/* Register this machine's init routine.  */

void _initialize_bee_tdep ();
void
_initialize_bee_tdep ()
{
  register_gdbarch_init (bfd_arch_bee, bee_gdbarch_init);
}
