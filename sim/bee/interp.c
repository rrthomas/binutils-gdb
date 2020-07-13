/* Simulator for the Bee processor
   Copyright (C) 2022 Free Software Foundation, Inc.
   Contributed by Reuben Thomas <rrt@sc3d.org>

This file is part of GDB, the GNU debugger.

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

/* This must come before any other includes.  */
#include "gnulib/config.h"

#include "defs.h"

#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "binary-io.h"
#include "verify.h"
#include "bfd.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-base.h"
#include "sim-options.h"
#include "sim-io.h"
#include "sim-signal.h"

#include "bee.h"
#include "private.h"
#include "opcode/bee.h"


static unsigned long
bee_extract_unsigned_integer (unsigned char *addr, int len)
{
  unsigned long retval;
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;
 
  if (len > (int) sizeof (unsigned long))
    printf ("That operation is not available on integers of more than %zu bytes.",
	    sizeof (unsigned long));
 
  /* Start at the least significant end of the integer, and work towards
     the most significant.  */
  retval = 0;

  for (p = startaddr; p < endaddr;)
    retval = (retval << 8) | * p ++;
  
  return retval;
}

static void
bee_store_unsigned_integer (unsigned char *addr, int len, unsigned long val)
{
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  for (p = startaddr; p < endaddr;)
    {
      * p ++ = val & 0xff;
      val >>= 8;
    }
}

/* OS support.  */
/* Assumption for file functions.  */
verify (sizeof(int) <= sizeof(bee_word_t));


/* The machine state.

   This state is maintained in host byte order.  The fetch/store
   register functions must translate between host byte order and the
   target processor byte order.  Keeping this data in target byte
   order simplifies the register read/write functions.  Keeping this
   data in native order improves the performance of the simulator.
   Simulation speed is deemed more important.  */

#define NUM_BEE_REGS 10
#define PC_REGNO     0
verify (MAX_NR_PROCESSORS == 1);

/* The ordering of the bee_regset structure is matched in the
   gdb/config/moxie/tm-moxie.h file in the REGISTER_NAMES macro.  */
/* TODO: This should be moved to sim-main.h:_sim_cpu.  */
struct bee_regset
{
  bee_uword_t		  regs[NUM_BEE_REGS]; /* primary registers */
  unsigned long long insts;           /* instruction counter */
};

/* TODO: This should be moved to sim-main.h:_sim_cpu.  */
union
{
  struct bee_regset asregs;
  bee_word_t asints [1];		/* but accessed larger... */
} cpu;

/* Write a 1 byte value to memory.  */

static INLINE void
wbat (sim_cpu *scpu, bee_word_t x, bee_word_t v)
{
  address_word cia = CPU_PC_GET (scpu);
  
  sim_core_write_aligned_1 (scpu, cia, write_map, x, v);
}

/* Write a 2 byte value to memory.  */

static INLINE void
wsat (sim_cpu *scpu, bee_word_t x, bee_word_t v)
{
  address_word cia = CPU_PC_GET (scpu);
  
  sim_core_write_aligned_2 (scpu, cia, write_map, x, v);
}

/* Write a 4 byte value to memory.  */

static INLINE void
wlat (sim_cpu *scpu, bee_word_t x, bee_word_t v)
{
  address_word cia = CPU_PC_GET (scpu);
	
  sim_core_write_aligned_4 (scpu, cia, write_map, x, v);
}

/* Write an 8-byte value to memory.  */

static INLINE void
wdat (sim_cpu *scpu, bee_word_t x, bee_word_t v)
{
  address_word cia = CPU_PC_GET (scpu);
	
  sim_core_write_aligned_8 (scpu, cia, write_map, x, v);
}

/* Read 1 byte from memory.  */

static INLINE int
rbat (sim_cpu *scpu, bee_word_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return sim_core_read_aligned_1 (scpu, cia, read_map, x);
}

/* Read 2 bytes from memory.  */

static INLINE int
rsat (sim_cpu *scpu, bee_word_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return sim_core_read_aligned_2 (scpu, cia, read_map, x);
}

/* Read 4 bytes from memory.  */

static INLINE int
rlat (sim_cpu *scpu, bee_word_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return sim_core_read_aligned_4 (scpu, cia, read_map, x);
}

/* Read 8 bytes from memory.  */

static INLINE bee_word_t
rdat (sim_cpu *scpu, bee_word_t x)
{
  address_word cia = CPU_PC_GET (scpu);
  
  return sim_core_read_aligned_8 (scpu, cia, read_map, x);
}

/* Read/write a word to/from memory.  */

#if BEE_WORD_BYTES == 4
#define rwat rlat
#define wwat wlat
#else
#define rwat rdat
#define wwat wdat
#endif


// Stacks

static INLINE int bee_pop_stack(sim_cpu *scpu, bee_uword_t s0, bee_uword_t ssize, bee_uword_t *sp, bee_word_t *val_ptr)
{
    if (unlikely(*sp == 0))
        return BEE_ERROR_STACK_UNDERFLOW;
    else if (unlikely(*sp > ssize))
        return BEE_ERROR_STACK_OVERFLOW;
    (*sp)--;
    *val_ptr = rwat (scpu, s0 + *sp * BEE_WORD_BYTES);
    return BEE_ERROR_OK;
}

static INLINE int bee_push_stack(sim_cpu *scpu, bee_uword_t s0, bee_uword_t ssize, bee_uword_t *sp, bee_word_t val)
{
    if (unlikely(*sp >= ssize))
        return BEE_ERROR_STACK_OVERFLOW;
    wwat (scpu, s0 + *sp * BEE_WORD_BYTES, val);
    (*sp)++;
    return BEE_ERROR_OK;
}


// I/O support

#if SIZEOF_INTPTR_T == 4
typedef uint64_t bee_duword_t;
#define PUSH_DOUBLE(ud)                                              \
    PUSHD((bee_uword_t)(ud & (bee_uword_t)-1));                      \
    PUSHD((bee_uword_t)((ud >> BEE_WORD_BIT) & (bee_uword_t)-1))
#define POP_DOUBLE(ud)                                                  \
    bee_word_t pop1, pop2;                                              \
    POPD(&pop1);                                                        \
    POPD(&pop2);                                                        \
    *ud = (((bee_duword_t)(bee_uword_t)pop1) << BEE_WORD_BIT | (bee_uword_t)pop2)
#else
typedef size_t bee_duword_t;
#define PUSH_DOUBLE(res)                        \
    PUSHD(res);                                 \
    PUSHD(0)
#define POP_DOUBLE(res)                         \
    POPD((bee_word_t *)res);                    \
    POPD((bee_word_t *)res)
#endif

// Command-line args
static int main_argc = 0;
static bee_uword_t main_argv = 0xf0000000;
static uint8_t *main_argv_buffer;

// FIXME: store 0 in m0 register, real M0 in a separate variable.
static void *virt2nat (bee_word_t addr)
{
  if (addr >= main_argv)
    return main_argv_buffer + (addr - main_argv);
  return (uint8_t *)cpu.asregs.regs[BEE_REG_m0] + addr;
}


static bee_word_t trap_libc(sim_cpu *scpu, bee_uword_t function)
{
  bee_word_t temp = 0;

  int error = BEE_ERROR_OK;
  switch (function) {
  case BEE_TRAP_LIBC_STRLEN: // ( a-addr -- u )
    {
      bee_uword_t s;
      POPD ((bee_word_t *)&s);
      PUSHD (strlen (virt2nat (s)));
    }
    break;
  case BEE_TRAP_LIBC_STRNCPY: // ( a-addr1 a-addr2 u -- a-addr3 )
    {
      bee_uword_t src, dest;
      size_t n;
      POPD ((bee_word_t *)&n);
      POPD ((bee_word_t *)&src);
      POPD ((bee_word_t *)&dest);
      PUSHD ((bee_word_t)(size_t)(void *)strncpy (virt2nat (dest), virt2nat (src), n));
    }
    break;
  case BEE_TRAP_LIBC_STDIN:
    PUSHD((bee_word_t)(STDIN_FILENO));
    break;
  case BEE_TRAP_LIBC_STDOUT:
    PUSHD((bee_word_t)(STDOUT_FILENO));
    break;
  case BEE_TRAP_LIBC_STDERR:
    PUSHD((bee_word_t)(STDERR_FILENO));
    break;
  case BEE_TRAP_LIBC_O_RDONLY:
    PUSHD((bee_word_t)(O_RDONLY));
    break;
  case BEE_TRAP_LIBC_O_WRONLY:
    PUSHD((bee_word_t)(O_WRONLY));
    break;
  case BEE_TRAP_LIBC_O_RDWR:
    PUSHD((bee_word_t)(O_RDWR));
    break;
  case BEE_TRAP_LIBC_O_CREAT:
    PUSHD((bee_word_t)(O_CREAT));
    break;
  case BEE_TRAP_LIBC_O_TRUNC:
    PUSHD((bee_word_t)(O_TRUNC));
    break;
  case BEE_TRAP_LIBC_OPEN: // ( c-addr flags -- fd )
    {
      int fd;
      bee_uword_t flags, file;
      POPD((bee_word_t *)&flags);
      POPD((bee_word_t *)&file);
      fd = open(virt2nat (file), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
      PUSHD((bee_word_t)fd);
      if (fd >= 0 && O_BINARY) {
	SET_BINARY(fd); // Best effort
      }
    }
    break;
  case BEE_TRAP_LIBC_CLOSE:
    {
      int fd;
      POPD(&temp);
      fd = temp;
      PUSHD((bee_word_t)close(fd));
    }
    break;
  case BEE_TRAP_LIBC_READ:
    {
      int fd;
      bee_uword_t nbytes, buf;
      POPD (&temp);
      fd = temp;
      POPD ((bee_word_t *)&nbytes);
      POPD ((bee_word_t *)&buf);
      PUSHD (read (fd, virt2nat (buf), nbytes));
    }
    break;
  case BEE_TRAP_LIBC_WRITE:
    {
      int fd;
      bee_uword_t nbytes, buf;
      POPD (&temp);
      fd = temp;
      POPD ((bee_word_t *)&nbytes);
      POPD ((bee_word_t *)&buf);
      PUSHD (write(fd, virt2nat(buf), nbytes));
    }
    break;
  case BEE_TRAP_LIBC_SEEK_SET:
    PUSHD((bee_word_t)(SEEK_SET));
    break;
  case BEE_TRAP_LIBC_SEEK_CUR:
    PUSHD((bee_word_t)(SEEK_CUR));
    break;
  case BEE_TRAP_LIBC_SEEK_END:
    PUSHD((bee_word_t)(SEEK_END));
    break;
  case BEE_TRAP_LIBC_LSEEK:
    {
      int fd, whence;
      bee_duword_t ud;
      off_t res;
      POPD(&temp);
      whence = temp;
      POP_DOUBLE(&ud);
      POPD(&temp);
      fd = temp;
      res = lseek(fd, (off_t)ud, whence);
      PUSH_DOUBLE((bee_duword_t)res);
    }
    break;
  case BEE_TRAP_LIBC_FDATASYNC:
    {
      int fd;
      POPD (&temp);
      fd = temp;
      PUSHD (fdatasync(fd));
    }
    break;
  case BEE_TRAP_LIBC_RENAME:
    {
      bee_uword_t to, from;
      POPD ((bee_word_t *)&to);
      POPD ((bee_word_t *)&from);
      PUSHD (rename(virt2nat (from), virt2nat (to)));
    }
    break;
  case BEE_TRAP_LIBC_REMOVE:
    {
      bee_uword_t file;
      POPD ((bee_word_t *)&file);
      PUSHD (remove (virt2nat (file)));
    }
    break;
  case BEE_TRAP_LIBC_FILE_SIZE:
    {
      int fd, res;
      struct stat st;
      POPD (&temp);
      fd = temp;
      res = fstat (fd, &st);
      PUSH_DOUBLE ((bee_duword_t)st.st_size);
      PUSHD (res);
    }
    break;
  case BEE_TRAP_LIBC_RESIZE_FILE:
    {
      int fd, res;
      bee_duword_t ud;
      POPD (&temp);
      fd = temp;
      POP_DOUBLE (&ud);
      res = ftruncate (fd, (off_t)ud);
      PUSHD (res);
    }
    break;
  case BEE_TRAP_LIBC_FILE_STATUS:
    {
      int fd, res;
      struct stat st;
      POPD (&temp);
      fd = temp;
      res = fstat (fd, &st);
      PUSHD (st.st_mode);
      PUSHD (res);
    }
    break;
  case BEE_TRAP_LIBC_ARGC: // ( -- u )
    PUSHD (main_argc);
    break;
  case BEE_TRAP_LIBC_ARGV: // ( -- a-addr )
    PUSHD ((bee_word_t)main_argv + BEE_WORD_BYTES);
    break;
  default:
    error = BEE_ERROR_INVALID_FUNCTION;
    break;
  }

 error:
  return error;
}

static bee_word_t trap(sim_cpu *scpu, bee_word_t code)
{
  int error = BEE_ERROR_OK;
  switch (code) {
  case BEE_TRAP_LIBC:
    {
      bee_uword_t function = 0;
      POPD((bee_word_t *)&function);
      return trap_libc(scpu, function);
    }
    break;
  default:
    return BEE_ERROR_INVALID_LIBRARY;
    break;
  }

 error:
  return error;
}


/* TODO: Split this up into finer trace levels than just insn.  */
#define BEE_TRACE_INSN(str) \
  TRACE_INSN (scpu, "0x%08x, %s, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx", \
	      opc, str, cpu.asregs.regs[0], cpu.asregs.regs[1], \
	      cpu.asregs.regs[2], cpu.asregs.regs[3], cpu.asregs.regs[4], \
	      cpu.asregs.regs[5], cpu.asregs.regs[6], cpu.asregs.regs[7], \
	      cpu.asregs.regs[8], cpu.asregs.regs[9])

// Address checking
#define CHECK_ALIGNED(a)                                        \
    if (!IS_ALIGNED(a))                                         \
        THROW(BEE_ERROR_UNALIGNED_ADDRESS);

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int siggnal) /* ignore  */
{
  bee_uword_t pc, old_pc;
  bee_word_t ir, error = BEE_ERROR_OK;
  unsigned opc;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */
  address_word cia = CPU_PC_GET (scpu);

  pc = cpu.asregs.regs[PC_REGNO];

  /* Run instructions here. */
  do 
    {
      /* Fetch the instruction at pc.  */
      ir = rwat (scpu, pc);
      old_pc = pc;
      pc += BEE_WORD_BYTES;

      /* Decode and execute instruction.  */
      opc = ir & BEE_OP1_MASK;
      switch (opc)
	{
	case BEE_OP_CALLI:
	  {
	    bee_uword_t addr;
	    BEE_TRACE_INSN ("CALLI");
	    PUSHS(pc);
	    addr = old_pc + ARSHIFT(ir, BEE_OP1_SHIFT) * BEE_WORD_BYTES;
	    CHECK_ALIGNED(addr);
	    pc = addr;
	  }
	  break;
	case BEE_OP_PUSHI:
	  BEE_TRACE_INSN ("PUSHI");
	  PUSHD(ARSHIFT(ir, BEE_OP1_SHIFT));
	  break;
	case BEE_OP_PUSHRELI:
	  BEE_TRACE_INSN ("PUSHRELI");
	  PUSHD(old_pc + ARSHIFT(ir, BEE_OP1_SHIFT) * BEE_WORD_BYTES);
	  break;
	default:
	  opc = ir & BEE_OP2_MASK;
	  switch (opc) {
	  case BEE_OP_JUMPI:
	    {
	      bee_uword_t addr;
	      BEE_TRACE_INSN ("JUMPI");
	      addr = old_pc + ARSHIFT(ir, BEE_OP2_SHIFT) * BEE_WORD_BYTES;
	      CHECK_ALIGNED(addr);
	      pc = addr;
	    }
	    break;
	  case BEE_OP_JUMPZI:
	    {
	      bee_uword_t addr;
	      bee_word_t flag;
	      BEE_TRACE_INSN ("JUMPZI");
	      addr = old_pc + ARSHIFT(ir, BEE_OP2_SHIFT) * BEE_WORD_BYTES;
	      POPD(&flag);
	      if (flag == 0) {
		CHECK_ALIGNED(addr);
		pc = addr;
	      }
	    }
	    break;
	  case BEE_OP_TRAP:
	    BEE_TRACE_INSN ("TRAP");
	    THROW_IF_ERROR(trap(scpu, (bee_uword_t)ir >> BEE_OP2_SHIFT));
	    break;
	  case BEE_OP_INSN:
	    {
	      bee_uword_t opcode = (bee_uword_t)ir >> BEE_OP2_SHIFT;
	      switch (opcode) {
	      case BEE_INSN_NOP:
		BEE_TRACE_INSN ("NOP");
		break;
	      case BEE_INSN_NOT:
		{
		  bee_word_t a;
		  BEE_TRACE_INSN ("NOT");
		  POPD(&a);
		  PUSHD(~a);
		}
		break;
	      case BEE_INSN_AND:
		{
		  bee_word_t a, b;
		  BEE_TRACE_INSN ("AND");
		  POPD(&a);
		  POPD(&b);
		  PUSHD(a & b);
		}
		break;
	      case BEE_INSN_OR:
		{
		  bee_word_t a, b;
		  BEE_TRACE_INSN ("OR");
		  POPD(&a);
		  POPD(&b);
		  PUSHD(a | b);
		}
		break;
	      case BEE_INSN_XOR:
		{
		  bee_word_t a, b;
		  BEE_TRACE_INSN ("XOR");
		  POPD(&a);
		  POPD(&b);
		  PUSHD(a ^ b);
		}
		break;
	      case BEE_INSN_LSHIFT:
		{
		  bee_word_t shift, value;
		  BEE_TRACE_INSN ("LSHIFT");
		  POPD(&shift);
		  POPD(&value);
		  PUSHD(shift < (bee_word_t)BEE_WORD_BIT ? LSHIFT(value, shift) : 0);
		}
		break;
	      case BEE_INSN_RSHIFT:
		{
		  bee_word_t shift, value;
		  BEE_TRACE_INSN ("RSHIFT");
		  POPD(&shift);
		  POPD(&value);
		  PUSHD(shift < (bee_word_t)BEE_WORD_BIT ? (bee_word_t)((bee_uword_t)value >> shift) : 0);
		}
		break;
	      case BEE_INSN_ARSHIFT:
		{
		  bee_word_t shift, value;
		  BEE_TRACE_INSN ("ARSHIFT");
		  POPD(&shift);
		  POPD(&value);
		  PUSHD(ARSHIFT(value, shift));
		}
		break;
	      case BEE_INSN_POP:
		BEE_TRACE_INSN ("POP");
		if (bee_R(dp) == 0)
		  THROW(BEE_ERROR_STACK_UNDERFLOW);
		bee_R(dp)--;
		break;
	      case BEE_INSN_DUP:
		{
		  bee_uword_t depth;
		  BEE_TRACE_INSN ("DUP");
		  POPD((bee_word_t *)&depth);
		  if (depth >= bee_R(dp))
		    THROW(BEE_ERROR_STACK_UNDERFLOW);
		  else
		    PUSHD(rwat (scpu, bee_R (d0) + (bee_R (dp) - (depth + 1)) * BEE_WORD_BYTES));
		}
		break;
	      case BEE_INSN_SET:
		{
		  bee_uword_t depth;
		  bee_word_t value;
		  BEE_TRACE_INSN ("SET");
		  POPD((bee_word_t *)&depth);
		  POPD(&value);
		  if (depth >= bee_R(dp))
		    THROW(BEE_ERROR_STACK_UNDERFLOW);
		  else
		    wwat (scpu, bee_R (d0) + (bee_R(dp) - (depth + 1)) * BEE_WORD_BYTES, value);
		}
		break;
	      case BEE_INSN_SWAP:
		{
		  bee_uword_t depth;
		  BEE_TRACE_INSN ("SWAP");
		  POPD((bee_word_t *)&depth);
		  if (bee_R(dp) == 0 || depth >= bee_R(dp) - 1)
		    THROW(BEE_ERROR_STACK_UNDERFLOW);
		  else {
		    bee_word_t temp = rwat (scpu, bee_R (d0) + (bee_R(dp) - (depth + 2)) * BEE_WORD_BYTES);
		    wwat (scpu, bee_R (d0) + (bee_R(dp) - (depth + 2)) * BEE_WORD_BYTES,
			  rwat (scpu, bee_R (d0) + (bee_R(dp) - 1) * BEE_WORD_BYTES));
		    wwat (scpu, bee_R (d0) + (bee_R(dp) - 1) * BEE_WORD_BYTES, temp);
		  }
		}
		break;
	      case BEE_INSN_JUMP:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("JUMP");
		  POPD((bee_word_t *)&addr);
		  CHECK_ALIGNED(addr);
		  pc = addr;
		}
		break;
	      case BEE_INSN_JUMPZ:
		{
		  bee_uword_t addr;
		  bee_word_t flag;
		  BEE_TRACE_INSN ("JUMPZ");
		  POPD((bee_word_t *)&addr);
		  POPD(&flag);
		  if (flag == 0) {
		    CHECK_ALIGNED(addr);
		    pc = addr;
		  }
		}
		break;
	      case BEE_INSN_CALL:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("CALL");
		  POPD((bee_word_t *)&addr);
		  CHECK_ALIGNED(addr);
		  PUSHS(pc);
		  pc = addr;
		}
		break;
	      case BEE_INSN_RET:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("RET");
		  POPS((bee_word_t *)&addr);
		  CHECK_ALIGNED(addr);
		  if (bee_R(sp) < bee_R(handler_sp)) {
		    POPS((bee_word_t *)&bee_R(handler_sp));
		    PUSHD(0);
		  }
		  pc = addr;
		}
		break;
	      case BEE_INSN_LOAD:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("LOAD");
		  POPD ((bee_word_t *)&addr);
		  CHECK_ALIGNED (addr);
		  PUSHD (rwat (scpu, addr));
		}
		break;
	      case BEE_INSN_STORE:
		{
		  bee_uword_t addr;
		  bee_word_t value;
		  BEE_TRACE_INSN ("STORE");
		  POPD ((bee_word_t *)&addr);
		  CHECK_ALIGNED (addr);
		  POPD (&value);
		  wwat (scpu, addr, value);
		}
		break;
	      case BEE_INSN_LOAD1:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("LOAD1");
		  POPD ((bee_word_t *)&addr);
		  PUSHD (rbat (scpu, addr));
		}
		break;
	      case BEE_INSN_STORE1:
		{
		  bee_uword_t addr;
		  bee_word_t value;
		  BEE_TRACE_INSN ("STORE1");
		  POPD ((bee_word_t *)&addr);
		  POPD (&value);
		  wbat (scpu, addr, value);
		}
		break;
	      case BEE_INSN_LOAD2:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("LOAD2");
		  POPD((bee_word_t *)&addr);
		  if ((bee_uword_t)addr % 2 != 0)
		    THROW(BEE_ERROR_UNALIGNED_ADDRESS);
		  PUSHD (rsat (scpu, addr));
		}
		break;
	      case BEE_INSN_STORE2:
		{
		  bee_uword_t addr;
		  bee_word_t value;
		  BEE_TRACE_INSN ("STORE2");
		  POPD ((bee_word_t *)&addr);
		  if ((bee_uword_t)addr % 2 != 0)
		    THROW(BEE_ERROR_UNALIGNED_ADDRESS);
		  POPD (&value);
		  wsat (scpu, addr, value);
		}
		break;
	      case BEE_INSN_LOAD4:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("LOAD4");
		  POPD ((bee_word_t *)&addr);
		  if ((bee_uword_t)addr % 4 != 0)
		    THROW(BEE_ERROR_UNALIGNED_ADDRESS);
		  PUSHD (rlat (scpu, addr));
		}
		break;
	      case BEE_INSN_STORE4:
		{
		  bee_uword_t addr;
		  bee_word_t value;
		  BEE_TRACE_INSN ("STORE4");
		  POPD ((bee_word_t *)&addr);
		  if ((bee_uword_t)addr % 4 != 0)
		    THROW(BEE_ERROR_UNALIGNED_ADDRESS);
		  POPD (&value);
		  wlat (scpu, addr, value);
		}
		break;
	      case BEE_INSN_NEG:
		{
		  bee_uword_t a;
		  BEE_TRACE_INSN ("NEG");
		  POPD((bee_word_t *)&a);
		  PUSHD((bee_word_t)-a);
		}
		break;
	      case BEE_INSN_ADD:
		{
		  bee_uword_t a, b;
		  BEE_TRACE_INSN ("ADD");
		  POPD((bee_word_t *)&a);
		  POPD((bee_word_t *)&b);
		  PUSHD((bee_word_t)(b + a));
		}
		break;
	      case BEE_INSN_MUL:
		{
		  bee_uword_t a, b;
		  BEE_TRACE_INSN ("MUL");
		  POPD((bee_word_t *)&a);
		  POPD((bee_word_t *)&b);
		  PUSHD((bee_word_t)(a * b));
		}
		break;
	      case BEE_INSN_DIVMOD:
		{
		  bee_word_t divisor, dividend;
		  BEE_TRACE_INSN ("DIVMOD");
		  POPD(&divisor);
		  POPD(&dividend);
		  if (dividend == BEE_WORD_MIN && divisor == -1) {
		    PUSHD(BEE_WORD_MIN);
		    PUSHD(0);
		  } else {
		    PUSHD(DIV_CATCH_ZERO(dividend, divisor));
		    PUSHD(MOD_CATCH_ZERO(dividend, divisor));
		  }
		}
		break;
	      case BEE_INSN_UDIVMOD:
		{
		  bee_uword_t divisor, dividend;
		  BEE_TRACE_INSN ("UDIVMOD");
		  POPD((bee_word_t *)&divisor);
		  POPD((bee_word_t *)&dividend);
		  PUSHD(DIV_CATCH_ZERO(dividend, divisor));
		  PUSHD(MOD_CATCH_ZERO(dividend, divisor));
		}
		break;
	      case BEE_INSN_EQ:
		{
		  bee_word_t a, b;
		  BEE_TRACE_INSN ("EQ");
		  POPD(&a);
		  POPD(&b);
		  PUSHD(a == b);
		}
		break;
	      case BEE_INSN_LT:
		{
		  bee_word_t a, b;
		  BEE_TRACE_INSN ("LT");
		  POPD(&a);
		  POPD(&b);
		  PUSHD(b < a);
		}
		break;
	      case BEE_INSN_ULT:
		{
		  bee_uword_t a, b;
		  BEE_TRACE_INSN ("ULT");
		  POPD((bee_word_t *)&a);
		  POPD((bee_word_t *)&b);
		  PUSHD(b < a);
		}
		break;
	      case BEE_INSN_PUSHS:
		{
		  bee_word_t value;
		  BEE_TRACE_INSN ("PUSHS");
		  POPD(&value);
		  PUSHS(value);
		}
		break;
	      case BEE_INSN_POPS:
		{
		  bee_word_t value;
		  BEE_TRACE_INSN ("POPS");
		  POPS(&value);
		  if (error == BEE_ERROR_OK)
		    PUSHD(value);
		}
		break;
	      case BEE_INSN_DUPS:
		BEE_TRACE_INSN ("DUPS");
		if (bee_R(sp) == 0)
		  THROW(BEE_ERROR_STACK_UNDERFLOW);
		else {
		  bee_word_t value = rwat (scpu, bee_R (s0) + (bee_R (sp) - 1) * BEE_WORD_BYTES);
		  PUSHD(value);
		}
		break;
	      case BEE_INSN_CATCH:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("CATCH");
		  POPD((bee_word_t *)&addr);
		  CHECK_ALIGNED(addr);
		  PUSHS(bee_R(handler_sp));
		  PUSHS(pc);
		  bee_R(handler_sp) = bee_R(sp);
		  pc = addr;
		}
		break;
	      case BEE_INSN_THROW:
		{
		  bee_uword_t addr;
		  BEE_TRACE_INSN ("THROW");
		  if (bee_R(dp) < 1)
		    error = BEE_ERROR_STACK_UNDERFLOW;
		  else
		    POPD(&error);
		error:
		  if (bee_R(handler_sp) == 0)
		    sim_engine_halt (sd, scpu, NULL, old_pc, sim_exited, error);
		  else {
		    // Don't push error code if the stack is full.
		    if (bee_R(dp) < bee_R(dsize))
		      wwat (scpu, bee_R (d0) + bee_R(dp)++ * BEE_WORD_BYTES, error);
		    bee_R(sp) = bee_R(handler_sp);
		    POPS((bee_word_t *)&addr);
		    POPS((bee_word_t *)&bee_R(handler_sp));
		    pc = addr;
		  }
		}
		break;
	      case BEE_INSN_BREAK:
		BEE_TRACE_INSN ("BREAK");
		sim_engine_halt (sd, scpu, NULL, pc, sim_stopped, SIM_SIGTRAP);
		break;
	      case BEE_INSN_WORD_BYTES:
		BEE_TRACE_INSN ("WORD_BYTES");
		PUSHD(BEE_WORD_BYTES);
		break;
	      case BEE_INSN_GET_SSIZE:
		BEE_TRACE_INSN ("GET_SSIZE");
		PUSHD(bee_R(ssize));
		break;
	      case BEE_INSN_GET_SP:
		BEE_TRACE_INSN ("GET_SP");
		PUSHD(bee_R(sp));
		break;
	      case BEE_INSN_SET_SP:
		BEE_TRACE_INSN ("SET_SP");
		POPD((bee_word_t *)&bee_R(sp));
		break;
	      case BEE_INSN_GET_DSIZE:
		BEE_TRACE_INSN ("GET_DSIZE");
		PUSHD(bee_R(dsize));
		break;
	      case BEE_INSN_GET_DP:
		{
		  bee_word_t value;
		  BEE_TRACE_INSN ("GET_DP");
		  value = bee_R(dp);
		  PUSHD(value);
		}
		break;
	      case BEE_INSN_SET_DP:
		{
		  bee_word_t value;
		  BEE_TRACE_INSN ("SET_DP");
		  POPD(&value);
		  bee_R(dp) = value;
		}
		break;
	      case BEE_INSN_GET_HANDLER_SP:
		BEE_TRACE_INSN ("GET_HANDLER_SP");
		PUSHD(bee_R(handler_sp));
		break;
	      default:
		BEE_TRACE_INSN ("INVALID");
		THROW(BEE_ERROR_INVALID_OPCODE);
		break;
	      }
	    }
	    break;
	  }
	}

      cpu.asregs.insts++;
      cpu.asregs.regs[PC_REGNO] = pc;

      if (sim_events_tick (sd))
	sim_events_process (sd);

    } while (1);
}

static int
bee_reg_store (SIM_CPU *scpu, int rn, unsigned char *memory, int length)
{
  if (rn < NUM_BEE_REGS && rn >= 0)
    {
      if (length == BEE_WORD_BYTES)
	{
	  long ival;
	  
	  /* misalignment safe */
	  ival = bee_extract_unsigned_integer (memory, BEE_WORD_BYTES);
	  cpu.asints[rn] = ival;
	}

      return BEE_WORD_BYTES;
    }
  else
    return 0;
}

static int
bee_reg_fetch (SIM_CPU *scpu, int rn, unsigned char *memory, int length)
{
  if (rn < NUM_BEE_REGS && rn >= 0)
    {
      if (length == BEE_WORD_BYTES)
	{
	  long ival = cpu.asints[rn];

	  /* misalignment-safe */
	  bee_store_unsigned_integer (memory, BEE_WORD_BYTES, ival);
	}
      
      return BEE_WORD_BYTES;
    }
  else
    return 0;
}

static sim_cia
bee_pc_get (sim_cpu *cpu)
{
  return cpu->registers[PCIDX];
}

static void
bee_pc_set (sim_cpu *cpu, sim_cia pc)
{
  cpu->registers[PCIDX] = pc;
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  int i;
  char *cmd;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_LITTLE; // FIXME: use native endism

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 1) != SIM_RC_OK
      || sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK
      || sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Create the memory.  */
  bee_R (msize) = 0x4000000;
  if (asprintf (&cmd, "memory-region 0x00000000,%#lx",
                bee_R (msize)) == -1)
    {
      free_state (sd);
      return 0;
    }
  sim_do_command (sd, cmd);
  free (cmd);
  
  if (asprintf (&cmd, "memory-region %#lx,0x1000000",
                main_argv) == -1)
    {
      free_state (sd);
      return 0;
    }
  sim_do_command (sd, cmd);
  free (cmd);

  {
    sim_core_mapping *map = CPU_CORE (STATE_CPU (sd, 0))->common.map[0].first->next;
    main_argv_buffer = map->buffer;
  }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  {
    int i;
    SIM_CPU *scpu = STATE_CPU (sd, 0);

    CPU_REG_FETCH (scpu) = bee_reg_fetch;
    CPU_REG_STORE (scpu) = bee_reg_store;
    CPU_PC_FETCH (scpu) = bee_pc_get;
    CPU_PC_STORE (scpu) = bee_pc_set;

    /* Clean out the register contents.  */
    cpu.asregs.regs[BEE_REG_pc] = 0;
    cpu.asregs.regs[BEE_REG_ssize] = 0;
    cpu.asregs.regs[BEE_REG_sp] = 0;
    cpu.asregs.regs[BEE_REG_dsize] = 0;
    cpu.asregs.regs[BEE_REG_dp] = 0;
    cpu.asregs.regs[BEE_REG_handler_sp] = 0;
  }

  {
    sim_core_mapping *map = CPU_CORE (STATE_CPU (sd, 0))->common.map[0].first;
    cpu.asregs.regs[BEE_REG_m0] = (bee_uword_t)map->buffer;
  }

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  int i;
  size_t argv_data_len = 0, argv_data_offset, tp;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */
  char *cmd;

  bee_R(ssize) = BEE_DEFAULT_STACK_SIZE;
  bee_R(dsize) = BEE_DEFAULT_STACK_SIZE;
  bee_R(s0) = bee_R(msize) - bee_R(ssize) * BEE_WORD_BYTES;
  bee_R(d0) = bee_R(s0) - bee_R(dsize) * BEE_WORD_BYTES;

  /* Count args.  */
  for (main_argc = 0; argv && argv[main_argc]; main_argc++)
    argv_data_len += strlen (argv[main_argc]) + 1;
  argv_data_offset = (main_argc + 2) * BEE_WORD_BYTES;

  /* Target memory words from 'main_argv' looks like this:
     argc
     start of argv
     ...
     end of argv
     0
     start of data pointed to by argv  */

  wwat (scpu, main_argv, main_argc);

  /* tp is the target address of the next argv item.  */
  tp = main_argv + argv_data_offset;

  for (i = 0; i < main_argc; i++)
    {
      size_t argv_item_len = strlen (argv[i]) + 1;

      /* Set the argv value.  */
      wwat (scpu, main_argv + (i + 1) * BEE_WORD_BYTES, tp);

      /* Store the string.  */
      sim_core_write_buffer (sd, scpu, write_map, argv[i],
			     tp, argv_item_len);
      tp += argv_item_len;
    }

  wlat (scpu, main_argv + (main_argc + 1) * BEE_WORD_BYTES, 0);

  if (prog_bfd != NULL)
    cpu.asregs.regs[PC_REGNO] = bfd_get_start_address (prog_bfd);

  return SIM_RC_OK;
}
