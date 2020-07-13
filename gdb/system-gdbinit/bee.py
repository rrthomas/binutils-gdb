# Extra GDB commands for Bee architecture.
#
# Copyright (C) 2020 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import gdb

# Utility functions and classes

def get_endianness():
    out = gdb.execute('show endian', False, True)
    return 'little' if out.find('little') >= 0 else 'big'

def set_register(reg, val):
    '''
    Set register `reg` to value `val`.
    '''
    gdb.execute('set $%s = %d' % (reg, val))


class Stack:
    '''
    Represent a Bee stack given by the registered named `base_register`,
    `size_register` and `pointer_register`.
    '''
    def __init__(self, base_register, size_register, pointer_register):
        self.base_register = base_register
        self.size_register = size_register
        self.pointer_register = pointer_register
        self.word_size = gdb.lookup_type('long').sizeof
        self.word_mask = (1 << (self.word_size * 8)) - 1

    @property
    def base(self):
        frame = gdb.selected_frame()
        return frame.read_register(self.base_register)

    @property
    def size(self):
        frame = gdb.selected_frame()
        return int(frame.read_register(self.size_register))

    def __len__(self):
        frame = gdb.selected_frame()
        return int(frame.read_register(self.pointer_register))

    def __getitem__(self, index):
        if not 0 <= index < len(self):
            raise IndexError
        return int.from_bytes(
            gdb.selected_inferior().read_memory(
                self.base + index * self.word_size,
                self.word_size
            ),
            get_endianness(),
        )

    def __setitem__(self, index, val):
        if not 0 <= index < len(self):
            raise IndexError
        gdb.selected_inferior().write_memory(
            self.base + index * self.word_size,
            int(val).to_bytes(self.word_size, get_endianness())
        )

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def __str__(self):
        signed_stack = list(self)
        unsigned_stack = map(lambda v: v & self.word_mask, signed_stack)
        return '  '.join(['%d (0x%x)' % (int(s), int(u)) for (s, u) in zip(signed_stack, unsigned_stack)])

    def append(self, val):
        if len(self) >= self.size:
            raise IndexError
        set_register(self.pointer_register, len(self) + 1)
        self[len(self) - 1] = val

    def pop(self):
        val = self[len(self) - 1]
        set_register(self.pointer_register, len(self) - 1)
        return val

data_stack = Stack('d0', 'dsize', 'dp')
return_stack = Stack('s0', 'ssize', 'sp')


class StackCommand(gdb.Command):
    '''
    Abstract class to define a command on stacks.
    '''
    def __init__(self, command_format, stack_name, stack):
        self.command_name = command_format % stack_name
        super().__init__(self.command_name, gdb.COMMAND_DATA)
        self.stack_name = stack_name
        self.stack = stack


class ShowStack(StackCommand):
    def __init__(self, stack_name, stack):
        super().__init__("show %s-stack", stack_name, stack)

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        if len(argv) != 0:
            raise gdb.GdbError('%s takes no arguments' % self.command_name)
        s = str(self.stack)
        print('%s stack%s' % (self.stack_name.capitalize(), ': ' + s if s != '' else ' is empty'))

ShowStack('data', data_stack)
ShowStack('return', return_stack)

# Useful aliases
gdb.execute('alias data=show data-stack')
gdb.execute('alias stack=show return-stack')


class PushStack(StackCommand):
    def __init__(self, stack_name, stack):
        super().__init__("push-%s", stack_name, stack)

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        if len(argv) != 1:
            raise gdb.GdbError('%s takes 1 argument' % self.command_name)
        try:
            val = int(gdb.parse_and_eval(argv[0]))
        except ValueError:
            raise gdb.GdbError('argument must be an integer')
        try:
            self.stack.append(val)
        except IndexError:
            print('%s stack is full' % self.stack_name.capitalize())

PushStack('data', data_stack)
PushStack('return', return_stack)

class PopStack(StackCommand):
    def __init__(self, stack_name, stack):
        super().__init__("pop-%s", stack_name, stack)

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        if len(argv) != 0:
            raise gdb.GdbError('%s takes no arguments' % self.command_name)
        try:
            print(int(self.stack.pop()))
        except IndexError:
            print('%s stack is empty' % self.stack_name.capitalize())

PopStack('data', data_stack)
PopStack('return', return_stack)
