# This script generates a Python interface for an Apple Macintosh Manager.
# It uses the "bgen" package to generate C code.
# The function specifications are generated by scanning the mamager's header file,
# using the "scantools" package (customized for this particular manager).

import string

# Declarations that change for each manager
MACHEADERFILE = 'Menus.h'		# The Apple header file
MODNAME = 'Menu'			# The name of the module
OBJECTNAME = 'Menu'			# The basic name of the objects used here

# The following is *usually* unchanged but may still require tuning
MODPREFIX = MODNAME			# The prefix for module-wide routines
OBJECTTYPE = OBJECTNAME + 'Handle'	# The C type used to represent them
OBJECTPREFIX = MODPREFIX + 'Obj'	# The prefix for object methods
INPUTFILE = string.lower(MODPREFIX) + 'gen.py' # The file generated by the scanner
EXTRAFILE = string.lower(MODPREFIX) + 'edit.py' # A similar file but hand-made
OUTPUTFILE = MODNAME + "module.c"	# The file generated by this program

from macsupport import *

# Create the type objects

MenuHandle = OpaqueByValueType(OBJECTTYPE, OBJECTPREFIX)
MenuRef = MenuHandle

unsigned_char = Type('unsigned char', 'b')

includestuff = includestuff + """
#include <Devices.h> /* Defines OpenDeskAcc in universal headers */
#include <%s>""" % MACHEADERFILE + """

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

#define as_Menu(h) ((MenuHandle)h)
"""

class MyObjectDefinition(GlobalObjectDefinition):
	pass

# Create the generator groups and link them
module = MacModule(MODNAME, MODPREFIX, includestuff, finalstuff, initstuff)
object = MyObjectDefinition(OBJECTNAME, OBJECTPREFIX, OBJECTTYPE)
module.addobject(object)

# Create the generator classes used to populate the lists
Function = OSErrFunctionGenerator
Method = OSErrMethodGenerator

# Create and populate the lists
functions = []
methods = []
execfile(INPUTFILE)
execfile(EXTRAFILE)

# add the populated lists to the generator groups
for f in functions: module.add(f)
for f in methods: object.add(f)

# generate output (open the output file as late as possible)
SetOutputFileName(OUTPUTFILE)
module.generate()
