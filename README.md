# Quick start for Bee binutils

Make sure you have bison, flex, texinfo, socat, expat, libgmp and libmpfr
and Python 3 installed (including development packages). Then run the
following commands to build and install binutils for Bee:

```
$ git clone --branch target-bee --depth 1 https://github.com/rrthomas/binutils-gdb
$ ./configure --target=bee-sc3d-elf --enable-sim --program-prefix=bee- --with-system-gdbinit-dir=/usr/local/share/gdb/system-gdbinit # add --with-python=/path/to/python3 if necessary
$ make && [sudo] make install
```

If you use a different `--prefix`, you will need to adjust
`--with-system-gdbinit-dir` to match.

Test that binutils is working:

```
$ cd binutils-gdb/sim/bee/tests
$ make test-binutils  # assembles and runs some code
$ bee-gdb
(gdb) target remote | bee --gdb hello.bin
```

Now you can try some gdb commands, for example:

```
(gdb) info registers
(gdb) disassemble $pc, $pc+64
(gdb) si              # step instruction: do this a few times
```
