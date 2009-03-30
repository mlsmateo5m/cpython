import os
import sys
import unittest
from test import test_support

ttk = test_support.import_module('ttk')
#If ttk exists _tkinter must exist.
from _tkinter import TclError

try:
    ttk.Button()
except TclError, msg:
    # assuming ttk is not available
    raise unittest.SkipTest("ttk not available: %s" % msg)

this_dir = os.path.dirname(os.path.abspath(__file__))
lib_tk_test = os.path.abspath(os.path.join(this_dir, os.path.pardir,
    'lib-tk', 'test'))
if lib_tk_test not in sys.path:
    sys.path.append(lib_tk_test)

import runtktests

def test_main(enable_gui=False):
    if enable_gui:
        if test_support.use_resources is None:
            test_support.use_resources = ['gui']
        elif 'gui' not in test_support.use_resources:
            test_support.use_resources.append('gui')

    test_support.run_unittest(
            *runtktests.get_tests(text=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main(enable_gui=True)
