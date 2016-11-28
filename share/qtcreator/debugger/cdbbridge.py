############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

import inspect
import os
import sys
import cdbext

sys.path.insert(1, os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))))

from dumper import *

class FakeVoidType(cdbext.Type):
    def __init__(self, name , dumper):
        cdbext.Type.__init__(self)
        self.typeName = name.strip()
        self.dumper = dumper

    def name(self):
        return self.typeName

    def bitsize(self):
        return self.dumper.ptrSize() * 8

    def code(self):
        if self.typeName.endswith('*'):
            return TypeCodePointer
        if self.typeName.endswith(']'):
            return TypeCodeArray
        return TypeCodeVoid

    def unqualified(self):
        return self

    def target(self):
        code = self.code()
        if code == TypeCodePointer:
            return FakeVoidType(self.typeName[:-1], self.dumper)
        if code == TypeCodeVoid:
            return self
        try:
            return FakeVoidType(self.typeName[:self.typeName.rindex('[')], self.dumper)
        except:
            return FakeVoidType('void', self.dumper)

    def stripTypedef(self):
        return self

    def fields(self):
        return []

    def templateArgument(self, pos, numeric):
        return None

    def templateArguments(self):
        return []

class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)
        self.outputLock = threading.Lock()
        self.isCdb = True

    def fromNativeValue(self, nativeValue):
        val = self.Value(self)
        val.name = nativeValue.name()
        val.nativeValue = nativeValue
        val.type = self.fromNativeType(nativeValue.type())
        val.isBaseClass = val.name == val.type.name
        val.lIsInScope = True
        val.laddress = nativeValue.address()
        return val

    def nativeTypeId(self, nativeType):
        self.check(isinstance(nativeType, cdbext.Type))
        name = nativeType.name()
        if name is None or len(name) == 0:
            c = '0'
        elif name == 'struct {...}':
            c = 's'
        elif name == 'union {...}':
            c = 'u'
        else:
            return name
        typeId = c + ''.join(['{%s:%s}' % (f.name(), self.nativeTypeId(f.type())) for f in nativeType.fields()])
        return typeId

    def fromNativeType(self, nativeType):
        self.check(isinstance(nativeType, cdbext.Type))
        code = nativeType.code()

        if nativeType.name().startswith('void'):
            nativeType = FakeVoidType(nativeType.name(), self)

        if code == TypeCodePointer:
            return self.createPointerType(self.fromNativeType(nativeType.target()))

        if code == TypeCodeArray:
            targetType = self.fromNativeType(nativeType.target())
            return self.createArrayType(targetType, nativeType.arrayElements())

        typeId = self.nativeTypeId(nativeType)
        if self.typeData.get(typeId, None) is None:
            tdata = self.TypeData(self)
            tdata.name = nativeType.name()
            tdata.typeId = typeId
            tdata.lbitsize = nativeType.bitsize()
            tdata.code = code
            self.registerType(typeId, tdata) # Prevent recursion in fields.
            if  code == TypeCodeStruct:
                tdata.lfields = lambda value : \
                    self.listFields(nativeType, value)
                tdata.lalignment = lambda : \
                    self.nativeStructAlignment(nativeType)
            tdata.templateArguments = self.listTemplateParameters(nativeType)
            self.registerType(typeId, tdata) # Fix up fields and template args
        return self.Type(self, typeId)

    def listFields(self, nativeType, value):
        if value.address() is None or value.address() == 0:
            raise Exception("")
        nativeValue = cdbext.createValue(value.address(), nativeType)
        index = 0
        nativeMember = nativeValue.childFromIndex(index)
        while nativeMember is not None:
            yield self.fromNativeValue(nativeMember)
            index += 1
            nativeMember = nativeValue.childFromIndex(index)

    def nativeStructAlignment(self, nativeType):
        #warn("NATIVE ALIGN FOR %s" % nativeType.name)
        def handleItem(nativeFieldType, align):
            a = self.fromNativeType(nativeFieldType).alignment()
            return a if a > align else align
        align = 1
        for f in nativeType.fields():
            align = handleItem(f.type(), align)
        return align

    def listTemplateParameters(self, nativeType):
        targs = []
        for targ in nativeType.templateArguments():
            if isinstance(targ, str):
                if self.typeData.get(targ, None) is None:
                    targs.append(self.lookupType(targ))
                else:
                    targs.append(self.Type(self, targ))
            elif isinstance(targ, int):
                targs.append(targ)
            else:
                error('CDBCRAP %s' % type(targ))
        return targs

    def nativeTypeEnumDisplay(self, nativeType, intval):
        # TODO: generate fake value
        return None

    def enumExpression(self, enumType, enumValue):
        ns = self.qtNamespace()
        return ns + "Qt::" + enumType + "(" \
            + ns + "Qt::" + enumType + "::" + enumValue + ")"

    def pokeValue(self, typeName, *args):
        return None

    def parseAndEvaluate(self, exp):
        val = cdbext.parseAndEvaluate(exp)
        if val is None:
            return None
        value = self.Value(self)
        value.type = self.lookupType('void *')
        value.ldata = val.to_bytes(8, sys.byteorder)
        return value

    def isWindowsTarget(self):
        return True

    def isQnxTarget(self):
        return False

    def isArmArchitecture(self):
        return False

    def isMsvcTarget(self):
        return True

    def qtHookDataSymbolName(self):
        return 'Qt5Cored#!qtHookData'

    def qtVersionAndNamespace(self):
        return ('', 0x50700) #FIXME: use a general approach in dumper or qttypes

    def qtNamespace(self):
        return self.qtVersionAndNamespace()[0]

    def qtVersion(self):
        return self.qtVersionAndNamespace()[1]

    def ptrSize(self):
        return cdbext.pointerSize()

    def put(self, stuff):
        self.output += stuff

    def lookupType(self, typeName):
        if len(typeName) == 0:
            return None
        if self.typeData.get(typeName, None) is None:
            nativeType = self.lookupNativeType(typeName)
            return None if nativeType is None else self.fromNativeType(nativeType)
        return self.Type(self, typeName)

    def lookupNativeType(self, name):
        if name.startswith('void'):
            return FakeVoidType(name, self)
        return cdbext.lookupType(name)

    def reportResult(self, result, args):
        self.report('result={%s}' % (result))

    def readRawMemory(self, address, size):
        mem = cdbext.readRawMemory(address, size)
        if len(mem) != size:
            raise Exception("Invalid memory request")
        return mem

    def findStaticMetaObject(self, typeName):
        ptr = self.findValueByExpression('&' + typeName + '::staticMetaObject')
        return ptr

    def warn(self, msg):
        self.put('{name="%s",value="",type="",numchild="0"},' % msg)

    def fetchVariables(self, args):
        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            self.reportResult(res, args)
            return

        self.setVariableFetchingOptions(args)

        self.output = ''

        self.currentIName = 'local'
        self.put('data=[')
        self.anonNumber = 0

        variables = []
        for val in cdbext.listOfLocals():
            self.currentContextValue = val
            name = val.name()
            value = self.fromNativeValue(val)
            value.name = name
            variables.append(value)

        self.handleLocals(variables)
        self.handleWatches(args)

        self.put('],partial="%d"' % (len(self.partialVariable) > 0))
        self.reportResult(self.output, args)

    def report(self, stuff):
        sys.stdout.write(stuff + "\n")

    def loadDumpers(self, args):
        msg = self.setupDumpers()
        self.reportResult(msg, args)

    def findValueByExpression(self, exp):
        return cdbext.parseAndEvaluate(exp)

    def nativeDynamicTypeName(self, address, baseType):
        return None # FIXME: Seems sufficient, no idea why.

    def callHelper(self, rettype, value, function, args):
        raise Exception("cdb does not support calling functions")

    def putCallItem(self, name, rettype, value, func, *args):
        return
