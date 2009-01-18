"""A pure Python implementation of import.

References on import:

    * Language reference
          http://docs.python.org/ref/import.html
    * __import__ function
          http://docs.python.org/lib/built-in-funcs.html
    * Packages
          http://www.python.org/doc/essays/packages.html
    * PEP 235: Import on Case-Insensitive Platforms
          http://www.python.org/dev/peps/pep-0235
    * PEP 275: Import Modules from Zip Archives
          http://www.python.org/dev/peps/pep-0273
    * PEP 302: New Import Hooks
          http://www.python.org/dev/peps/pep-0302/
    * PEP 328: Imports: Multi-line and Absolute/Relative
          http://www.python.org/dev/peps/pep-0328

"""
from . import _bootstrap

# XXX Temporary functions that should eventually be removed.
import os
import re
import tokenize

def _set__import__():
    """Set __import__ to an instance of Import."""
    global original__import__
    original__import__ = __import__
    __builtins__['__import__'] = Import()


def _reset__import__():
    """Set __import__ back to the original implementation (assumes
    _set__import__ was called previously)."""
    __builtins__['__import__'] = original__import__


def _case_ok(directory, check):
    """Check if the directory contains something matching 'check'.

    No check is done if the file/directory exists or not.

    """
    if 'PYTHONCASEOK' in os.environ:
        return True
    elif check in os.listdir(directory):
        return True
    return False


def _w_long(x):
    """Convert a 32-bit integer to little-endian.

    XXX Temporary until marshal's long functions are exposed.

    """
    x = int(x)
    int_bytes = []
    int_bytes.append(x & 0xFF)
    int_bytes.append((x >> 8) & 0xFF)
    int_bytes.append((x >> 16) & 0xFF)
    int_bytes.append((x >> 24) & 0xFF)
    return bytearray(int_bytes)


def _r_long(int_bytes):
    """Convert 4 bytes in little-endian to an integer.

    XXX Temporary until marshal's long function are exposed.

    """
    x = int_bytes[0]
    x |= int_bytes[1] << 8
    x |= int_bytes[2] << 16
    x |= int_bytes[3] << 24
    return x


def import_module(name, package=None):
    """Import a module.

    The 'package' argument is used to resolve relative import names. Typically
    this is the __package__ attribute of the module making the function call.

    """
    if name.startswith('.'):
        if not package:
            raise TypeError("relative imports require the 'package' argument")
        level = 0
        for character in name:
            if character != '.':
                break
            level += 1
        name = Import._resolve_name(name[level:], package, level)
    __import__(name)
    return sys.modules[name]



# Required built-in modules.
try:
    import posix as _os
except ImportError:
    try:
        import nt as _os
    except ImportError:
        try:
            import os2 as _os
        except ImportError:
            raise ImportError('posix, nt, or os2 module required for importlib')
_bootstrap._os = _os
import imp, sys, marshal, errno, _fileio
_bootstrap.imp = imp
_bootstrap.sys = sys
_bootstrap.marshal = marshal
_bootstrap.errno = errno
_bootstrap._fileio = _fileio
import _warnings
_bootstrap._warnings = _warnings


from os import sep
# For os.path.join replacement; pull from Include/osdefs.h:SEP .
_bootstrap.path_sep = sep

_bootstrap._case_ok = _case_ok
marshal._w_long = _w_long
marshal._r_long = _r_long

from ._bootstrap import *
