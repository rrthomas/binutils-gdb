	.data
# .tfloat is 80-bit floating point format.
	.tfloat 3.32192809488736218171e0
#	.byte 0x0, 0x88, 0x1b, 0xcd, 0x4b, 0x78, 0x9a, 0xd4, 0x0, 0x40
# .double is 64-bit floating point format.
	.double 3.32192809488736218171e0
#	.byte 0x71, 0xa3, 0x79, 0x09, 0x4f, 0x93, 0x0a, 0x40
# The next two are 32-bit floating point format.
	.float 3.32192809488736218171e0
#	.byte 0x78, 0x9a, 0x54, 0x40
	.single 3.32192809488736218171e0
#	.byte 0x78, 0x9a, 0x54, 0x40
	.p2align 4,0

# The assembler used to treat the next value as zero instead of 1e-22.
        .double .0000000000000000000001
        .double 1e-22
# The assembler used to limit the number of digits too much.
	.double 37778931862957165903871.0
	.double 37778931862957165903873.0
# Ensure we handle a crazy number of digits
	.double 1.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
	.p2align 4,0

	.ds.x 1, -1
	.p2align 4,0xcc
