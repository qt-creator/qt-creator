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

class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)
        self.outputLock = threading.Lock()
        self.isCdb = True

    def fromNativeValue(self, nativeValue):
        val = self.Value(self)
        val.nativeValue = nativeValue
        val.type = self.fromNativeType(nativeValue.type())
        val.lIsInScope = True
        val.laddress = nativeValue.address()
        return val

    def fromNativeType(self, nativeType):
        typeobj = self.Type(self)
        typeobj.nativeType = nativeType
        typeobj.name = nativeType.name()
        typeobj.lbitsize = nativeType.bitsize()
        typeobj.code = nativeType.code()
        return typeobj

    def nativeTypeFields(self, nativeType):
        fields = []
        for nativeField in nativeType.fields():
            field = self.Field(self)
            field.name = nativeField.name()
            field.parentType = self.fromNativeType(nativeType)
            field.ltype = self.fromNativeType(nativeField.type())
            field.lbitsize = nativeField.bitsize()
            field.lbitpos = nativeField.bitpos()
            fields.append(field)
        return fields

    def nativeTypeUnqualified(self, nativeType):
        return self.fromNativeType(nativeType.unqualified())

    def nativeTypePointer(self, nativeType):
        return self.fromNativeType(nativeType.target())

    def nativeTypeStripTypedefs(self, typeobj):
        return self.fromNativeType(nativeType.stripTypedef())

    def nativeTypeFirstBase(self, nativeType):
        return None

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
        return cdbext.parseAndEvaluate(exp)

    def isWindowsTarget(self):
        return True

    def isQnxTarget(self):
        return False

    def isArmArchitecture(self):
        return False

    def isMsvcTarget(self):
        return True

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

    def lookupNativeType(self, name):
        return cdbext.lookupType(name)

    def reportResult(self, result, args):
        self.report('result={%s}' % (result))

    def readRawMemory(self, address, size):
        return cdbext.readRawMemory(address, size)

    def findStaticMetaObject(self, typeName):
        symbolName = self.mangleName(typeName + '::staticMetaObject')
        symbol = self.target.FindFirstGlobalVariable(symbolName)
        return symbol.AddressOf().GetValueAsUnsigned() if symbol.IsValid() else 0

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
