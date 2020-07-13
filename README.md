# Quick start for Bee binutils

Make sure you have bison and flex installed, then run the following commands
to build and install binutils for Bee:

```
$ git clone --branch target-bee --depth 1 https://github.com/rrthomas/binutils-gdb
$ cd binutils-gdb/sim
$ git clone https://github.com/rrthomas/bee
$ cd bee
$ ./configure CC="gcc -m32 -O2 -g" && make && make check && [sudo] make install
$ cd ../..
$ ./configure --target=bee-sc3d-elf --program-prefix=bee- --with-system-gdbinit-dir=/usr/local/share/gdb/system-gdbinit
$ make && [sudo] make install
```

If you use a different `--prefix`, you will need to adjust
`--with-system-gdbinit-dir` to match. Note that binutils’s build system
doesn’t work with make running in parallel mode.

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

If you do `si` several times, gdb will hang. This is because Bee is trying
to output `Hello, world!` to standard output, which it is also using to
communicate with gdb. To give Bee a separate channel of communication with
gdb, a more complicated setup is needed. In `bee/tests`, in one
terminal, run:

```
$ socat unix-listen:./socket exec:'bee --gdb=3\,4 hello.bin',fdin=3,fdout=4
```

This tells Bee to use file descriptors 3 & 4 to talk to gdb, and socat then
connects these to a listening UNIX domain socket. Then, in another terminal:

```
$ bee-gdb
(gdb) target remote ./socket
(gdb) c
```

Then Bee prints `Hello, world!`.
