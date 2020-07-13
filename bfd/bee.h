#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/bee.h"

/* Map BFD reloc types to BEE ELF reloc types.  */

struct bee_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned int bee_reloc_val;
};

extern reloc_howto_type bee_elf_howto_table [9];
extern const struct bee_reloc_map bee_reloc_map [9];
reloc_howto_type *
bee_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
                       bfd_reloc_code_real_type code);
reloc_howto_type *
bee_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name);
