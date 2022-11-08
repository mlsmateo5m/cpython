# This script lists the names of standard library modules
# to update Python/stdlib_mod_names.h
import _imp
import os.path
import re
import subprocess
import sys
import sysconfig

from check_extension_modules import ModuleChecker


SCRIPT_NAME = 'Tools/build/generate_stdlib_module_names.py'

SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
STDLIB_PATH = os.path.join(SRC_DIR, 'Lib')

IGNORE = {
    '__init__',
    '__pycache__',
    'site-packages',

    # Test modules and packages
    '__hello__',
    '__phello__',
    '__hello_alias__',
    '__phello_alias__',
    '__hello_only__',
    '_ctypes_test',
    '_testbuffer',
    '_testcapi',
    '_testconsole',
    '_testimportmultiple',
    '_testinternalcapi',
    '_testmultiphase',
    '_testsinglephase',
    '_xxsubinterpreters',
    '_xxtestfuzz',
    'idlelib.idle_test',
    'test',
    'xxlimited',
    'xxlimited_35',
    'xxsubtype',
}

# Pure Python modules (Lib/*.py)
def list_python_modules(names):
    for filename in os.listdir(STDLIB_PATH):
        if not filename.endswith(".py"):
            continue
        name = filename.removesuffix(".py")
        names.add(name)


# Packages in Lib/
def list_packages(names):
    for name in os.listdir(STDLIB_PATH):
        if name in IGNORE:
            continue
        package_path = os.path.join(STDLIB_PATH, name)
        if not os.path.isdir(package_path):
            continue
        if any(package_file.endswith(".py")
               for package_file in os.listdir(package_path)):
            names.add(name)


# Built-in and extension modules built by Modules/Setup*
# includes Windows and macOS extensions.
def list_modules_setup_extensions(names):
    checker = ModuleChecker()
    names.update(checker.list_module_names(all=True))


# List frozen modules of the PyImport_FrozenModules list (Python/frozen.c).
# Use the "./Programs/_testembed list_frozen" command.
def list_frozen(names):
    submodules = set()
    for name in _imp._frozen_module_names():
        # To skip __hello__, __hello_alias__ and etc.
        if name.startswith('__'):
            continue
        if '.' in name:
            submodules.add(name)
        else:
            names.add(name)
    # Make sure all frozen submodules have a known parent.
    for name in list(submodules):
        if name.partition('.')[0] in names:
            submodules.remove(name)
    if submodules:
        raise Exception(f'unexpected frozen submodules: {sorted(submodules)}')


def list_modules():
    names = set(sys.builtin_module_names)
    list_modules_setup_extensions(names)
    list_packages(names)
    list_python_modules(names)
    list_frozen(names)

    # Remove ignored packages and modules
    for name in list(names):
        package_name = name.split('.')[0]
        # package_name can be equal to name
        if package_name in IGNORE:
            names.discard(name)

    for name in names:
        if "." in name:
            raise Exception("sub-modules must not be listed")

    return names


def write_modules(fp, names):
    print(f"// Auto-generated by {SCRIPT_NAME}.",
          file=fp)
    print("// List used to create sys.stdlib_module_names.", file=fp)
    print(file=fp)
    print("static const char* _Py_stdlib_module_names[] = {", file=fp)
    for name in sorted(names):
        print(f'"{name}",', file=fp)
    print("};", file=fp)


def main():
    if not sysconfig.is_python_build():
        print(f"ERROR: {sys.executable} is not a Python build",
              file=sys.stderr)
        sys.exit(1)

    fp = sys.stdout
    names = list_modules()
    write_modules(fp, names)


if __name__ == "__main__":
    main()
