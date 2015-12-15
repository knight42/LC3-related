#!/usr/bin/python -O
# -*- coding: utf-8 -*-
from __future__ import print_function, unicode_literals, division

import os
import glob
import filecmp
import subprocess

def judge(test_dir):
    for f in glob.glob(os.path.join(test_dir, '*.asm')):
        print('Verifying {} ...'.format(f))
        std_assmbler = os.path.join(test_dir, 'lc3as-std')

        sym = f.replace('.asm', '.sym')
        std_sym = f.replace('.asm', '.std.sym')

        obj = f.replace('.asm', '.obj')
        std_obj = f.replace('.asm', '.std.obj')

        subprocess.call([std_assmbler, f], stdout=subprocess.DEVNULL)
        os.rename(sym, std_sym)
        os.rename(obj, std_obj)

        subprocess.call([os.path.join(os.curdir, 'lc3as'), f], stdout=subprocess.DEVNULL)

        if not filecmp.cmp(sym, std_sym):
            print('Failed!')
            print('{} and {} differs!'.format(sym, std_sym))
            return 1

        if not filecmp.cmp(obj, std_obj):
            print('Failed!')
            print('{} and {} differs!'.format(obj, std_obj))
            return 1

        print('Pass!')
        for i in (sym, std_sym, obj, std_obj):
            os.remove(i)
    else:
        return 0

if __name__ == '__main__':
    judge('verify')
