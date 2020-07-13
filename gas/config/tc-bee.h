/* tc-bee.h -- Header file for tc-bee.c.

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

   You should have received a copy of the GNU General Public License along
   with GAS; see the file COPYING.  If not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#define TC_BEE 1

#ifndef TARGET_BYTES_BIG_ENDIAN
#define TARGET_BYTES_BIG_ENDIAN 0
#endif

#define WORKING_DOT_WORD

/* This macro is the BFD architecture to pass to `bfd_set_arch_mach'.  */
#define TARGET_FORMAT bee_target_format ()

#define TARGET_ARCH bfd_arch_bee

#define md_undefined_symbol(NAME)           0

/* These macros must be defined, but it will be a fatal assembler
   error if we ever hit them.  */
#define md_estimate_size_before_relax(A, B) (as_fatal (_("estimate size\n")), 0)
#define md_convert_frag(B, S, F)            as_fatal (_("convert_frag\n"))

#define md_section_align(SEGMENT, SIZE)     (SIZE)

void md_assemble_bee64 (char *str);

const char *bee_target_format (void);
