// Public data structures and interface calls specified in the VM definition.
// This is the header file to include in programs using an embedded VM.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU General Public License version 3,
// or, at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef BEE_BEE
#define BEE_BEE


#include <stdint.h>
#include <limits.h>


// Basic types
typedef intptr_t bee_word_t;
typedef uintptr_t bee_uword_t;
#define BEE_WORD_BYTES 8
#define BEE_WORD_BIT (BEE_WORD_BYTES * 8)
#define BEE_WORD_MIN INTPTR_MIN
#define BEE_UWORD_MAX UINTPTR_MAX

// VM registers
#define R(reg, type) BEE_REG_ ## reg,
enum {
#include "opcodes/bee-regs.h"
};
#undef R

#define bee_R(reg) (cpu.asregs.regs[BEE_REG_ ## reg])

// Error codes
enum {
    BEE_ERROR_OK = 0,
    BEE_ERROR_INVALID_OPCODE = -1,
    BEE_ERROR_STACK_UNDERFLOW = -2,
    BEE_ERROR_STACK_OVERFLOW = -3,
    BEE_ERROR_UNALIGNED_ADDRESS = -4,
    BEE_ERROR_INVALID_LIBRARY = -16,
    BEE_ERROR_INVALID_FUNCTION = -17,
    BEE_ERROR_BREAK = -256,
};

// Default stacks size in words
#define BEE_DEFAULT_STACK_SIZE   4096


#endif
