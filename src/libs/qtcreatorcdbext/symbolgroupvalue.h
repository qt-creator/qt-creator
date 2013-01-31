/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SYMBOLGROUPVALUE_H
#define SYMBOLGROUPVALUE_H

#include "common.h"
#include "knowntype.h"

#include <string>
#include <vector>
#include <list>

class AbstractSymbolGroupNode;
class SymbolGroupNode;
class SymbolGroup;
class MemoryHandle;

struct SymbolGroupValueContext
{
    SymbolGroupValueContext(CIDebugDataSpaces *ds, CIDebugSymbols *s) : dataspaces(ds), symbols(s) {}
    SymbolGroupValueContext::SymbolGroupValueContext() : dataspaces(0), symbols(0) {}

    CIDebugDataSpaces *dataspaces;
    CIDebugSymbols *symbols;
};

class SymbolGroupValue
{
    explicit SymbolGroupValue(const std::string &parentError);

public:
    typedef std::pair<std::string, ULONG64> Symbol;
    typedef std::list<Symbol> SymbolList;

    explicit SymbolGroupValue(SymbolGroupNode *node, const SymbolGroupValueContext &c);
    SymbolGroupValue();

    operator bool() const { return isValid(); }
    bool isValid() const;

    // Access children by name or index (0-based)
    SymbolGroupValue operator[](const char *name) const;
    SymbolGroupValue operator[](unsigned) const;
    unsigned childCount() const;
    SymbolGroupValue parent() const;
    // take address and cast to desired (pointer) type
    SymbolGroupValue typeCast(const char *type) const;
    // take pointer value and cast to desired (pointer) type
    SymbolGroupValue pointerTypeCast(const char *type) const;
    // Find a member variable traversing the list of base classes. This useful
    // for skipping template base classes of STL containers whose number varies
    // by MSVC version.
    static SymbolGroupValue findMember(const SymbolGroupValue &start,
                                       const std::string &symbolName);
    std::string name() const;
    std::string type() const;
    std::vector<std::string>  innerTypes() const { return innerTypesOf(type()); }
    std::wstring value() const;
    unsigned size() const;

    SymbolGroupNode *node() const { return m_node; }
    SymbolGroupValueContext context() const { return m_context; }

    int intValue(int defaultValue = -1) const;
    double floatValue(double defaultValue = -999) const;
    ULONG64 pointerValue(ULONG64 defaultValue = 0) const;
    ULONG64 address() const;
    std::string module() const;

    // Return allocated array of data pointed to
    unsigned char *pointerData(unsigned length) const;
    // Return data pointed to as wchar_t/std::wstring (UTF16)
    std::wstring wcharPointerData(unsigned charCount, unsigned maxCharCount = 512) const;

    std::string error() const;

    // Some helpers for manipulating types.
    static unsigned sizeOf(const char *type);
    // Offset of structure field: "!moduleQMapNode<K,T>", "value".
    static unsigned fieldOffset(const char *type, const char *field);
    static std::string stripPointerType(const std::string &);
    // Strip "class ", "struct "
    static std::string stripClassPrefixes(const std::string &);
    // Strip " const" from "char const*", (map key), "QString const", etc.
    // which otherwise causes GetTypeSize to fail.
    static std::string stripConst(const std::string &type);
    static std::string addPointerType(const std::string &);
    static std::string stripArrayType(const std::string &);
    static std::string stripModuleFromType(const std::string &type);
    static std::string moduleOfType(const std::string &type);
    // pointer type, return number of characters to strip
    static unsigned isPointerType(const std::string &);
    static bool isArrayType(const std::string &);
    static bool isVTableType(const std::string &t);
    // add pointer type 'Foo' -> 'Foo *', 'Foo *' -> 'Foo **'
    static std::string pointerType(const std::string &type);
    // Symbol Name/(Expression) of a pointed-to instance ('Foo' at 0x10') ==> '*(Foo *)0x10'
    static std::string pointedToSymbolName(ULONG64 address, const std::string &type);
    // Resolve a type, that is, obtain its module name ('QString'->'QtCored4!QString')
    // Some operations on types (like adding symbols may fail non-deterministically
    // or be slow when the module specification is omitted).
    // If a current SymbolGroup is passed on, its module will be used for templates.
    static std::string resolveType(const std::string &type,
                                   const SymbolGroupValueContext &ctx,
                                   const std::string &currentModule = std::string());

    static std::list<std::string> resolveSymbolName(const char *pattern,
                                                    const SymbolGroupValueContext &c,
                                                    std::string *errorMessage = 0);
    static SymbolList resolveSymbol(const char *pattern,
                                    const SymbolGroupValueContext &c,
                                    std::string *errorMessage = 0);

    static unsigned char *readMemory(CIDebugDataSpaces *ds, ULONG64 address, ULONG length,
                                     std::string *errorMessage = 0);
    static ULONG64 readPointerValue(CIDebugDataSpaces *ds, ULONG64 address,
                                    std::string *errorMessage = 0);
    static ULONG64 readUnsignedValue(CIDebugDataSpaces *ds,
                                     ULONG64 address, ULONG debuggeeTypeSize, ULONG64 defaultValue = 0,
                                     std::string *errorMessage = 0);
    static int readIntValue(CIDebugDataSpaces *ds,
                            ULONG64 address, int defaultValue = 0,
                            std::string *errorMessage = 0);
    static double readDouble(CIDebugDataSpaces *ds,
                             ULONG64 address, double defaultValue = 0.0,
                             std::string *errorMessage = 0);

    static bool writeMemory(CIDebugDataSpaces *ds, ULONG64 address,
                            const unsigned char *data, ULONG length,
                            std::string *errorMessage = 0);

    static unsigned pointerSize();
    static unsigned pointerDiffSize();
    static unsigned intSize();

    // get the inner types: "QMap<int, double>" -> "int", "double"
    static std::vector<std::string> innerTypesOf(const std::string &t);

    static unsigned verbose;

private:
    bool ensureExpanded() const;
    SymbolGroupValue typeCastedValue(ULONG64 address, const char *type) const;

    SymbolGroupNode *m_node;
    SymbolGroupValueContext m_context;
    mutable std::string m_errorMessage;
};

// For debugging purposes
std::ostream &operator<<(std::ostream &, const SymbolGroupValue &v);

struct QtInfo
{
    enum Module
    {
        Core, Gui, Widgets, Network, Script
    };

    QtInfo() : version(0) {}

    static const QtInfo &get(const SymbolGroupValueContext &ctx);

    // Prepend core module and Qt namespace. To be able to work with some
    // 'complicated' types like QMapNode, specifying the module helps
    std::string prependQtModule(const std::string &type, Module m = Core) const
        { return QtInfo::prependModuleAndNameSpace(type, moduleName(m), nameSpace); }

    std::string prependQtCoreModule(const std::string &type) const
        { return prependQtModule(type, Core); }
    std::string prependQtGuiModule(const std::string &type) const
        { return QtInfo::prependQtModule(type, Gui); }
    std::string prependQtWidgetsModule(const std::string &type) const
        { return QtInfo::prependQtModule(type, Widgets); }
    std::string prependQtNetworkModule(const std::string &type) const
        { return QtInfo::prependQtModule(type, Network); }
    std::string prependQtScriptModule(const std::string &type) const
        { return QtInfo::prependQtModule(type, Script); }

    // Prepend module and namespace if missing with some smartness
    // ('Foo' or -> 'nsp::Foo') => 'QtCored4!nsp::Foo'
    static std::string prependModuleAndNameSpace(const std::string &type,
                                                 const std::string &module,
                                                 const std::string &nameSpace);

    std::string moduleName(Module m) const;

    int version;
    std::string nameSpace;
    std::string libInfix;
    // Fully qualified types with module and namespace
    std::string qObjectType;
    std::string qObjectPrivateType;
    std::string qWindowPrivateType;
    std::string qWidgetPrivateType;
};

std::ostream &operator<<(std::ostream &os, const QtInfo &);

/* Helpers for detecting types reported from IDebugSymbolGroup
 * 1) Class prefix==true is applicable to outer types obtained from
 *    from IDebugSymbolGroup: 'class foo' or 'struct foo'.
 * 2) Class prefix==false is for inner types of templates, etc, doing
 *    a more expensive check:  'foo' */
enum
{
    KnownTypeHasClassPrefix = 0x1,   // Strip "class Foo" -> "Foo"
    KnownTypeAutoStripPointer = 0x2  // Strip "class Foo *" -> "Foo" for conveniently
                                     // handling the pointer/value duality of symbol group entries
};

KnownType knownType(const std::string &type, unsigned flags);

// Debug helper
void formatKnownTypeFlags(std::ostream &os, KnownType kt);

// Dump builtin simple types using SymbolGroupValue expressions,
// returning SymbolGroupNode dumper flags. Might return special info
// (for example, a cached additional node) for some types to be used in
// complex dumpers
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx,
                        std::wstring *s,
                        int *knownType = 0,
                        int *containerSizeIn = 0,
                        void **specialInfoIn = 0,
                        MemoryHandle **memoryHandleIn = 0);

bool dumpEditValue(const SymbolGroupNode *n, const SymbolGroupValueContext &,
                   int desiredFormat, std::ostream &str);

enum AssignEncoding
{
    AssignPlainValue,
    AssignHexEncoded,
    AssignHexEncodedUtf16
};

bool assignType(SymbolGroupNode  *n, int valueEncoding, const std::string &value,
                const SymbolGroupValueContext &ctx,
                std::string *errorMessage);

// Non-container complex dumpers (QObjects/QVariants).
std::vector<AbstractSymbolGroupNode *>
    dumpComplexType(SymbolGroupNode *node, int type, void *specialInfo,
                    const SymbolGroupValueContext &ctx);

#endif // SYMBOLGROUPVALUE_H
