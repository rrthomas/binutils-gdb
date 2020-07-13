// Private implementation-specific APIs that are shared between modules.
//
// (c) Reuben Thomas 1994-2022
//
// The package is distributed under the GNU General Public License version 3,
// or, at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

// Optimization
// Hint that `x` is usually true/false.
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


// Errors
#define THROW(code)                             \
    do {                                        \
        error = code;                           \
        goto error;                             \
    } while (0)

#define THROW_IF_ERROR(code)                    \
    do {                                        \
        bee_word_t _err = code;                 \
        if (_err != BEE_ERROR_OK)               \
            THROW(_err);                        \
    } while (0)


// Stack access
#define POPD(ptr)                                                       \
    THROW_IF_ERROR(bee_pop_stack(scpu, bee_R (d0), bee_R(dsize), &bee_R(dp), ptr))
#define PUSHD(val)                                                      \
    THROW_IF_ERROR(bee_push_stack(scpu, bee_R (d0), bee_R(dsize), &bee_R(dp), val))

#define POPS(ptr)                                                       \
    THROW_IF_ERROR(bee_pop_stack(scpu, bee_R (s0), bee_R(ssize), &bee_R(sp), ptr))
#define PUSHS(val)                                                      \
    THROW_IF_ERROR(bee_push_stack(scpu, bee_R (s0), bee_R(ssize), &bee_R(sp), val))


// Memory access

// Align a VM address
#define ALIGN(a)                                                \
    (((bee_uword_t)(a) + BEE_WORD_BYTES - 1) & (-BEE_WORD_BYTES))

// Check whether a VM address is aligned
#define IS_ALIGNED(a)                                           \
    (((uint8_t *)((bee_uword_t)(a) & (BEE_WORD_BYTES - 1)) == 0))


// Portable left shift (the behaviour of << with overflow (including on any
// negative number) is undefined)
#define LSHIFT(n, p)                            \
    (((n) & (BEE_UWORD_MAX >> (p))) << (p))


// Portable arithmetic right shift (the behaviour of >> on signed
// quantities is implementation-defined in C99)
#if HAVE_ARITHMETIC_RSHIFT
#define ARSHIFT(n, p)                           \
    ((bee_word_t)(n) >> (p))
#else
#define ARSHIFT(n, p)                                                   \
    (((n) >> (p)) | (bee_word_t)(LSHIFT(-((n) < 0), (BEE_WORD_BIT - p))))
#endif
