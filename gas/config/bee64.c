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

#define BEE_WORD_BYTES 8
#define BEE_WORD_BIT 64
#define BEE_RELOC_PCREL_LONG BFD_RELOC_BEE_61_PCREL
#define BEE_RELOC_PCREL_SHORT BFD_RELOC_BEE_61_PCREL
#define BEE_RELOC_LONG BFD_RELOC_BEE_61
#define BEENN_(name) bee64 ## name
#define _BEENN(name) name ## bee64

#include "beenn.h"
