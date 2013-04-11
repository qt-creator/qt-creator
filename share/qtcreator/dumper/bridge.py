
import binascii
import inspect
import os
import traceback

cdbLoaded = False
lldbLoaded = False
gdbLoaded = False

#######################################################################
#
# Helpers
#
#######################################################################

currentDir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
#print "DIR: %s " % currentDir

def warn(message):
    print "XXX: %s\n" % message.encode("latin1")

PointerCode = None
ArrayCode = None
StructCode = None
UnionCode = None
EnumCode = None
FlagsCode = None
FunctionCode = None
IntCode = None
FloatCode = None
VoidCode = None
SetCode = None
RangeCode = None
StringCode = None
BitStringCode = None
ErrorTypeCode = None
MethodCode = None
MethodPointerCode = None
MemberPointerCode = None
ReferenceCode = None
CharCode = None
BoolCode = None
ComplexCode = None
TypedefCode = None
NamespaceCode = None
SimpleValueCode = None # LLDB only

failReasons = []

# CDB
if False:
    try:
        import cdb_bridge
        cdbLoaded = True
    except:
        failReasons.append(traceback.format_exc())

# GDB
if not cdbLoaded:
    try:
        import gdb
        execfile(os.path.join(currentDir, "gbridge.py"))
        gdbLoaded = True
    except:
        failReasons.append(traceback.format_exc())


# LLDB
if not gdbLoaded and not cdbLoaded:
    #try:
        execfile(os.path.join(currentDir, "lbridge.py"))
        lldbLoaded = True
    #except:
    #    failReasons.append(traceback.format_exc())


# One is sufficient.
#if cdbLoaded or gdbLoaded or lldbLoaded:
#    failReasons = []

try:
    execfile(os.path.join(currentDir, "dumper.py"))
    execfile(os.path.join(currentDir, "qttypes.py"))
    bbsetup()
except:
    failReasons.append(traceback.format_exc())


if len(failReasons):
    print "CANNOT ACCESS ANY DEBUGGER BACKEND:\n %s" % "\n".join(failReasons)


