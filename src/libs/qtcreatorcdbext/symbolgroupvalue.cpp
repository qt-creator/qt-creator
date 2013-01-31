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

// std::copy is perfectly fine, don't let MSVC complain about it being deprecated
#pragma warning (disable: 4996)

#ifndef NOMINMAX
#  define NOMINMAX
#endif

#include "symbolgroupvalue.h"
#include "symbolgroup.h"
#include "stringutils.h"
#include "containers.h"
#include "extensioncontext.h"

#include <iomanip>
#include <algorithm>
#include <limits>
#include <ctype.h>

typedef std::vector<int>::size_type VectorIndexType;

/*! \struct SymbolGroupValueContext
    \brief Structure to pass all IDebug interfaces required for SymbolGroupValue
    \ingroup qtcreatorcdbext */

/*! \class SymbolGroupValue

    Flyweight tied to a SymbolGroupNode
    providing a convenient operator[] (name, index) and value
    getters for notation of dumpers.
    Inaccessible members return a SymbolGroupValue in state 'invalid'.
    Example:
    \code
    SymbolGroupValue container(symbolGroupNode, symbolGroupValueContext);
    if (SymbolGroupValue sizeV = container["d"]["size"])
      int size = sizeV.intValue()
    \endcode
    etc. \ingroup qtcreatorcdbext */

unsigned SymbolGroupValue::verbose = 0;

SymbolGroupValue::SymbolGroupValue(const std::string &parentError) :
    m_node(0), m_errorMessage(parentError)
{
    if (m_errorMessage.empty())
        m_errorMessage = "Invalid";
}

SymbolGroupValue::SymbolGroupValue(SymbolGroupNode *node,
                                   const SymbolGroupValueContext &ctx) :
    m_node(node), m_context(ctx)
{
    if (m_node && !m_node->isMemoryAccessible()) { // Invalid if no value
        m_node = 0;
        if (SymbolGroupValue::verbose)
            DebugPrint() << node->name() << '/' << node->iName() << '/'
                         << node->type() << " memory access error";
    }
}

SymbolGroupValue::SymbolGroupValue() :
    m_node(0), m_errorMessage("Invalid")
{
}

bool SymbolGroupValue::isValid() const
{
    return m_node != 0 && m_context.dataspaces != 0;
}

// Debug helper
static void formatNodeError(const AbstractSymbolGroupNode *n, std::ostream &os)
{
    const AbstractSymbolGroupNode::AbstractSymbolGroupNodePtrVector &children = n->children();
    const VectorIndexType size = children.size();
    if (const SymbolGroupNode *sn = n->asSymbolGroupNode()) {
        os << "type: " << sn->type() << ", raw value: \"" << wStringToString(sn->symbolGroupRawValue())
           << "\", 0x" << std::hex << sn->address() << ", " << std::dec;
    }
    if (size) {
        os << "children (" << size << "): [";
        for (VectorIndexType i = 0; i < size; ++i)
            os << ' ' << children.at(i)->name();
        os << ']';
    } else {
        os << "No children";
    }
}

SymbolGroupValue SymbolGroupValue::operator[](unsigned index) const
{
    if (ensureExpanded())
        if (index < m_node->children().size())
            if (SymbolGroupNode *n = m_node->childAt(index)->asSymbolGroupNode())
                return SymbolGroupValue(n, m_context);
    if (isValid() && SymbolGroupValue::verbose) {
        DebugPrint dp;
        dp << name() << "::operator[](#" << index << ") failed. ";
        if (m_node)
            formatNodeError(m_node, dp);
    }
    return SymbolGroupValue(m_errorMessage);
}

unsigned SymbolGroupValue::childCount() const
{
    if (ensureExpanded())
        return unsigned(m_node->children().size());
    return 0;
}

SymbolGroupValue SymbolGroupValue::parent() const
{
    if (isValid())
        if (AbstractSymbolGroupNode *aParent = m_node->parent())
            if (SymbolGroupNode *parent = aParent->asSymbolGroupNode())
                return SymbolGroupValue(parent, m_context);
    return SymbolGroupValue("parent() invoked on invalid value.");
}

bool SymbolGroupValue::ensureExpanded() const
{
    if (!isValid() || !m_node->canExpand())
        return false;

    if (m_node->isExpanded())
        return true;

    // Set a flag indicating the node was expanded by SymbolGroupValue
    // and not by an explicit request from the watch model.
    if (m_node->expand(&m_errorMessage)) {
        m_node->addFlags(SymbolGroupNode::ExpandedByDumper);
        return true;
    }
    if (SymbolGroupValue::verbose)
        DebugPrint() << "Expand failure of '" << name() << "': " << m_errorMessage;
    return false;
}

SymbolGroupValue SymbolGroupValue::operator[](const char *name) const
{
    if (ensureExpanded())
        if (AbstractSymbolGroupNode *child = m_node->childByIName(name))
            if (SymbolGroupNode *n = child->asSymbolGroupNode())
                return SymbolGroupValue(n, m_context);
    if (isValid() && SymbolGroupValue::verbose) { // Do not report subsequent errors
        DebugPrint dp;
        dp << this->name() << "::operator[](\"" << name << "\") failed. ";
        if (m_node)
            formatNodeError(m_node, dp);
    }
    return SymbolGroupValue(m_errorMessage);
}

std::string SymbolGroupValue::type() const
{
    return isValid() ? m_node->type() : std::string();
}

std::string SymbolGroupValue::name() const
{
    return isValid() ? m_node->name() : std::string();
}

unsigned SymbolGroupValue::size() const
{
    return isValid() ? m_node->size() : 0;
}

std::wstring SymbolGroupValue::value() const
{
    return isValid() ? m_node->symbolGroupFixedValue() : std::wstring();
}

double SymbolGroupValue::floatValue(double defaultValue) const
{
    double f = defaultValue;
    if (isValid()) {
        std::wistringstream str(value());
        str >> f;
        if (str.fail())
            f = defaultValue;
    }
    return f;
}

int SymbolGroupValue::intValue(int defaultValue) const
{
    if (isValid()) {
        int rc = 0;
        // Is this an enumeration "EnumValue (0n12)", -> convert to integer
        std::wstring v = value();
        const std::wstring::size_type enPos = v.find(L"(0n");
        if (enPos != std::wstring::npos && v.at(v.size() - 1) == L')')
            v = v.substr(enPos + 3, v.size() - 4);
        if (integerFromString(wStringToString(v), &rc))
            return rc;
    }
    if (SymbolGroupValue::verbose)
        DebugPrint() << name() << '/' << type() << '/'<< m_errorMessage << ": intValue() fails";
    return defaultValue;
}

ULONG64 SymbolGroupValue::pointerValue(ULONG64 defaultValue) const
{
    if (isValid()) {
        ULONG64 rc = 0;
        if (integerFromString(wStringToString(value()), &rc))
            return rc;
    }
    if (SymbolGroupValue::verbose)
        DebugPrint() << name() << '/'<< type() << '/' << m_errorMessage << ": pointerValue() fails";
    return defaultValue;
}

// Read a POD value from debuggee memory and convert into host variable type POD.
// For unsigned integer types, it is possible to read a smaller debuggee-unsigned
// into a big ULONG64 on the host side (due to endianness).
template<class POD>
    POD readPODFromMemory(CIDebugDataSpaces *ds, ULONG64 address, ULONG debuggeeTypeSize,
                          POD defaultValue, std::string *errorMessage /* = 0 */)
{
    POD rc = defaultValue;
    if (debuggeeTypeSize == 0 || debuggeeTypeSize > sizeof(POD)) // Safety check.
        return rc;
    if (const unsigned char *buffer = SymbolGroupValue::readMemory(ds, address, debuggeeTypeSize, errorMessage)) {
        memcpy(&rc, buffer, debuggeeTypeSize);
        delete [] buffer;
    }
    return rc;
}

ULONG64 SymbolGroupValue::readPointerValue(CIDebugDataSpaces *ds, ULONG64 address,
                                           std::string *errorMessage /* = 0 */)
{
    return readPODFromMemory<ULONG64>(ds, address, SymbolGroupValue::pointerSize(), 0, errorMessage);
}

ULONG64 SymbolGroupValue::readUnsignedValue(CIDebugDataSpaces *ds,
                                            ULONG64 address, ULONG debuggeeTypeSize,
                                            ULONG64 defaultValue,
                                            std::string *errorMessage /* = 0 */)
{
    return readPODFromMemory<ULONG64>(ds, address, debuggeeTypeSize, defaultValue, errorMessage);
}

// Note: sizeof(int) should always be 4 on Win32/Win64, no need to
// differentiate between host and debuggee int types. When implementing
// something like 'long', different size on host/debuggee must be taken
// into account  (shifting signedness bits around).
int SymbolGroupValue::readIntValue(CIDebugDataSpaces *ds,
                                   ULONG64 address, int defaultValue,
                                   std::string *errorMessage /* = 0 */)
{
    return readPODFromMemory<int>(ds, address, sizeof(int),
                                  defaultValue, errorMessage);
}

double SymbolGroupValue::readDouble(CIDebugDataSpaces *ds, ULONG64 address, double defaultValue,
                                    std::string *errorMessage /* = 0 */)
{
    return readPODFromMemory<double>(ds, address, sizeof(double), defaultValue, errorMessage);
}

// Read memory and return allocated array
unsigned char *SymbolGroupValue::readMemory(CIDebugDataSpaces *ds, ULONG64 address, ULONG length,
                                            std::string *errorMessage /* = 0 */)
{
    unsigned char *buffer = new unsigned char[length];
    std::fill(buffer, buffer + length, 0);
    ULONG received = 0;
    const HRESULT hr = ds->ReadVirtual(address, buffer, length, &received);
    if (FAILED(hr)) {
        delete [] buffer;
        if (errorMessage) {
            std::ostringstream estr;
            estr << "Cannot read " << length << " bytes from 0x"
                 << std::hex << address << ": "
                 << msgDebugEngineComFailed("ReadVirtual", hr);
            *errorMessage = estr.str();
        }
        return 0;
    }
    if (received < length && errorMessage) {
        std::ostringstream estr;
        estr << "Warning: Received only " << received
             << " bytes of " << length
             << " requested at 0x" << std::hex << address << '.';
        *errorMessage = estr.str();
    }
    return buffer;
}

bool SymbolGroupValue::writeMemory(CIDebugDataSpaces *ds, ULONG64 address,
                                   const unsigned char *data, ULONG length,
                                   std::string *errorMessage /* =0 */)
{
    ULONG filled = 0;
    const HRESULT hr = ds->FillVirtual(address, length,
                                       const_cast<unsigned char *>(data),
                                       length, &filled);
    if (FAILED(hr)) {
        if (errorMessage) {
            std::ostringstream estr;
            estr << "Cannot write " << length << " bytes to 0x"
                 << std::hex << address << ": "
                 << msgDebugEngineComFailed("FillVirtual", hr);
            *errorMessage = estr.str();
        }
        return false;
    }
    if (filled < length) {
        if (errorMessage) {
            std::ostringstream estr;
            estr << "Filled only " << filled << " bytes of " << length
                 << " at 0x" << std::hex << address << '.';
            *errorMessage = estr.str();
        }
        return false;
    }
    return true;
}

// Return allocated array of data
unsigned char *SymbolGroupValue::pointerData(unsigned length) const
{
    if (isValid())
        if (const ULONG64 ptr = pointerValue())
            return SymbolGroupValue::readMemory(m_context.dataspaces, ptr, length);
    return 0;
}

ULONG64 SymbolGroupValue::address() const
{
    if (isValid())
        return m_node->address();
    return 0;
}

std::string SymbolGroupValue::module() const
{
    return isValid() ? m_node->module() : std::string();
}

// Temporary iname
static inline std::string additionalSymbolIname(const SymbolGroup *g)
{
    std::ostringstream str;
    str << "__additional" << g->root()->children().size();
    return str.str();
}

SymbolGroupValue SymbolGroupValue::typeCast(const char *type) const
{
    return typeCastedValue(address(), type);
}

SymbolGroupValue SymbolGroupValue::pointerTypeCast(const char *type) const
{
    return typeCastedValue(pointerValue(), type);
}

SymbolGroupValue SymbolGroupValue::typeCastedValue(ULONG64 address, const char *type) const
{
    if (!address)
        return SymbolGroupValue();
    const size_t len = strlen(type);
    if (!len)
        return SymbolGroupValue();
    const bool nonPointer = type[len - 1] != '*';
    SymbolGroup *sg = m_node->symbolGroup();
    // A bit of magic: For desired pointer types, we can do
    //     'Foo *' -> '(Foo *)(address)'.
    // For non-pointers, we need to de-reference:
    //      'Foo' -> '*(Foo *)(address)'
    std::ostringstream str;
    if (nonPointer)
        str << '*';
    str << '(' << type;
    if (nonPointer)
        str << " *";
    str << ")(" << std::showbase << std::hex << address << ')';
    if (SymbolGroupNode *node = sg->addSymbol(module(), str.str(),
                                              additionalSymbolIname(sg),
                                              &m_errorMessage))
        return SymbolGroupValue(node, m_context);
    return SymbolGroupValue();
}

SymbolGroupValue SymbolGroupValue::findMember(const SymbolGroupValue &start,
                                              const std::string &symbolName)
{
    const unsigned count = start.childCount();
    for (unsigned i = 0; i < count ; ++i) { // check members
        const SymbolGroupValue child = start[i];
        if (child.name() == symbolName)
            return child;
    }
    // Recurse down base classes at the beginning that have class names
    // as value.
    for (unsigned i = 0; i < count; ++i) {
        const SymbolGroupValue child = start[i];
        const std::wstring value = child.value();
        if (value.compare(0, 6, L"class ") && value.compare(0, 7, L"struct ")) {
           break;
        } else {
            if (SymbolGroupValue r = findMember(child, symbolName))
                return r;
        }
    }
    return SymbolGroupValue();
}

std::wstring SymbolGroupValue::wcharPointerData(unsigned charCount, unsigned maxCharCount) const
{
    const bool truncated = charCount > maxCharCount;
    if (truncated)
        charCount = maxCharCount;
    if (const unsigned char *ucData = pointerData(charCount * sizeof(wchar_t))) {
        const wchar_t *utf16Data = reinterpret_cast<const wchar_t *>(ucData);
        // Be smart about terminating 0-character
        if (charCount && utf16Data[charCount - 1] == 0)
            charCount--;
        std::wstring rc = std::wstring(utf16Data, charCount);
        if (truncated)
            rc += L"...";
        delete [] ucData;
        return rc;
    }
    return std::wstring();
}

std::string SymbolGroupValue::error() const
{
    return m_errorMessage;
}

// Return number of characters to strip for pointer type
unsigned SymbolGroupValue::isPointerType(const std::string &t)
{
    if (endsWith(t, "**"))
        return 1;
    if (endsWith(t, " *"))
        return 2;
    return 0;
}

bool SymbolGroupValue::isArrayType(const std::string &t)
{
    return endsWith(t, ']');
}

// Return number of characters to strip for pointer type
bool SymbolGroupValue::isVTableType(const std::string &t)
{
    const char vtableTypeC[] = "__fptr()";
    return t.compare(0, sizeof(vtableTypeC) - 1, vtableTypeC) == 0;
}

// add pointer type 'Foo' -> 'Foo *', 'Foo *' -> 'Foo **'
std::string SymbolGroupValue::pointerType(const std::string &type)
{
    std::string rc = type;
    if (!endsWith(type, '*'))
        rc.push_back(' ');
    rc.push_back('*');
    return rc;
}

unsigned SymbolGroupValue::pointerSize()
{
    static const unsigned ps = SymbolGroupValue::sizeOf("char *");
    return ps;
}

unsigned SymbolGroupValue::intSize()
{
    static const unsigned is = SymbolGroupValue::sizeOf("int");
    return is;
}

unsigned SymbolGroupValue::pointerDiffSize()
{
    static const unsigned is = SymbolGroupValue::sizeOf("ptrdiff_t");
    return is;
}

unsigned SymbolGroupValue::sizeOf(const char *type)
{
    const unsigned rc = GetTypeSize(type);
    if (!rc && SymbolGroupValue::verbose)
        DebugPrint() << "GetTypeSize fails for '" << type << '\'';
    return rc;
}

unsigned SymbolGroupValue::fieldOffset(const char *type, const char *field)
{
    ULONG rc = 0;
    if (GetFieldOffset(type, field, &rc)) {
        if (SymbolGroupValue::verbose)
            DebugPrint() << "GetFieldOffset fails for '" << type << "' '" << field << '\'';
        return 0;
    }
    return rc;
}

std::string SymbolGroupValue::stripPointerType(const std::string &t)
{
    // 'Foo *' -> 'Foo', 'Bar **' -> 'Bar *'.
    if (const unsigned stripLength = isPointerType(t))
        return t.substr(0, t.size() - stripLength);
    return t;
}

// Strip "class Foo", "struct Bar"-> "Foo", "Bar "
std::string SymbolGroupValue::stripClassPrefixes(const std::string &type)
{
    std::string rc = type;
    if (rc.compare(0, 6, "class ") == 0) {
        rc.erase(0, 6);
    } else {
        if (rc.compare(0, 7, "struct ") == 0)
            rc.erase(0, 7);
    }
    return rc;
}

// Strip " const" from end of type ("XX const", "XX const *[*]"
std::string SymbolGroupValue::stripConst(const std::string &type)
{
    const std::string::size_type constPos = type.rfind(" const");
    if (constPos == std::string::npos)
        return type;
    // Strip 'const' only if it is at the end 'QString const'
    // or of some pointer like 'foo ***'
    std::string rc = type;
    const std::string::size_type size = rc.size();
    std::string::size_type nextPos = constPos + 6;
    if (nextPos == size) { // Ends with - easy.
        rc.erase(constPos, nextPos - constPos);
        return rc;
    }
    // Ensure it ends with ' ****'.
    if (rc.at(nextPos) != ' ')
        return rc;
    for (std::string::size_type i = nextPos + 1; i < size; ++i)
        if (rc.at(i) != '*')
            return rc;
    rc.erase(constPos, nextPos - constPos);
    return rc;
}

std::string SymbolGroupValue::addPointerType(const std::string &t)
{
    // 'char' -> 'char *' -> 'char **'
    std::string rc = t;
    if (!endsWith(rc, '*'))
        rc.push_back(' ');
    rc.push_back('*');
    return rc;
}

std::string SymbolGroupValue::stripArrayType(const std::string &t)
{
    const std::string::size_type bracket = t.rfind('[');
    if (bracket != std::string::npos) {
        std::string rc = t.substr(0, bracket);
        trimBack(rc);
        return rc;
    }
    return t;
}

std::string SymbolGroupValue::stripModuleFromType(const std::string &type)
{
    const std::string::size_type exclPos = type.find('!');
    return exclPos != std::string::npos ? type.substr(exclPos + 1, type.size() - exclPos - 1) : type;
}

std::string SymbolGroupValue::moduleOfType(const std::string &type)
{
    const std::string::size_type exclPos = type.find('!');
    return exclPos != std::string::npos ? type.substr(0, exclPos) : std::string();
}

// Symbol Name/(Expression) of a pointed-to instance ('Foo' at 0x10') ==> '*(Foo *)0x10'
std::string SymbolGroupValue::pointedToSymbolName(ULONG64 address, const std::string &type)
{
    std::ostringstream str;
    str << "*(" << type;
    if (!endsWith(type, '*'))
        str << ' ';
    str << "*)" << std::showbase << std::hex << address;
    return str.str();
}

/* QtInfo helper: Determine the full name of a Qt Symbol like 'qstrdup' in 'QtCored4'.
 * as 'QtCored4![namespace::]qstrdup'. In the event someone really uses a different
 * library prefix or namespaced Qt, this should be found.
 * The crux is here that the underlying IDebugSymbols::StartSymbolMatch()
 * does not accept module wildcards (as opposed to the 'x' command where
 * 'x QtCo*!*qstrdup' would be acceptable and fast). OTOH, doing a wildcard search
 * like '*qstrdup' is very slow and should be done only if there is really a
 * different namespace or lib prefix.
 * Parameter 'modulePatternC' is used to do a search on the modules returned
 * (due to the amiguities and artifacts that appear like 'QGuid4!qstrdup'). */

static inline std::string resolveQtSymbol(const char *symbolC,
                                          const char *moduleNameC,
                                          const SymbolGroupValueContext &ctx)
{
    enum { debugResolveQtSymbol =  0 };
    typedef std::list<std::string> StringList;
    typedef StringList::const_iterator StringListConstIt;

    if (debugResolveQtSymbol)
        DebugPrint() << ">resolveQtSymbol" << symbolC << " def=" << moduleNameC << " defModName="
                     << moduleNameC;
    const SubStringPredicate modulePattern(moduleNameC);
    // First try a match with the default module name 'QtCored4!qstrdup' or
    //  'Qt5Cored!qstrdup' for speed reasons.
    for (int qtVersion = 4; qtVersion < 6; qtVersion++) {
        std::ostringstream str;
        str << "Qt";
        if (qtVersion >= 5)
            str << qtVersion;
        str << moduleNameC << 'd';
        if (qtVersion == 4)
            str << qtVersion;
        str << '!' << symbolC;
        const std::string defaultPattern = str.str();
        const StringList defaultMatches = SymbolGroupValue::resolveSymbolName(defaultPattern.c_str(), ctx);
        if (debugResolveQtSymbol)
            DebugPrint() << "resolveQtSymbol: defaultMatches=" << qtVersion
                         << DebugSequence<StringListConstIt>(defaultMatches.begin(), defaultMatches.end());
        const StringListConstIt defaultIt = std::find_if(defaultMatches.begin(), defaultMatches.end(), modulePattern);
        if (defaultIt != defaultMatches.end()) {
            if (debugResolveQtSymbol)
                DebugPrint() << "<resolveQtSymbol return1 " << *defaultIt;
            return *defaultIt;
        }
    }
    // Fail, now try a search with '*qstrdup' in all modules. This might return several matches
    // like 'QtCored4!qstrdup', 'QGuid4!qstrdup'
    const std::string wildCardPattern = std::string(1, '*') + symbolC;
    const StringList allMatches = SymbolGroupValue::resolveSymbolName(wildCardPattern.c_str(), ctx);
    if (debugResolveQtSymbol)
        DebugPrint() << "resolveQtSymbol: allMatches= (" << wildCardPattern << ") -> " << DebugSequence<StringListConstIt>(allMatches.begin(), allMatches.end());
    const StringListConstIt allIt = std::find_if(allMatches.begin(), allMatches.end(), modulePattern);
    const std::string rc = allIt != allMatches.end() ? *allIt : std::string();
    if (debugResolveQtSymbol)
        DebugPrint() << "<resolveQtSymbol return2 " << rc;
    return rc;
}

/*! \struct QtInfo

    Qt Information determined on demand: Namespace, modules and basic class
    names containing the module for fast lookup.
    \ingroup qtcreatorcdbext */

const QtInfo &QtInfo::get(const SymbolGroupValueContext &ctx)
{
    static QtInfo rc;
    if (!rc.libInfix.empty())
        return rc;

    do {
        // Lookup qstrdup() to hopefully get module (potential libinfix) and namespace
        // Typically, this resolves to 'QtGuid4!qstrdup' and 'QtCored4!qstrdup'...
        const std::string qualifiedSymbol = resolveQtSymbol("qstrdup", "Core", ctx);
        const std::string::size_type libPos = qualifiedSymbol.find("Core");
        const std::string::size_type exclPos = qualifiedSymbol.find('!'); // Resolved: 'QtCored4!qstrdup'
        if (libPos == std::string::npos || exclPos == std::string::npos) {
            rc.libInfix = "d4";
            rc.version = 4;
            break;
        }
        rc.libInfix = qualifiedSymbol.substr(libPos + 4, exclPos - libPos - 4);
        // 'Qt5Cored!qstrdup' or 'QtCored4!qstrdup'.
        if (isdigit(qualifiedSymbol.at(2)))
            rc.version = qualifiedSymbol.at(2) - '0';
        else
            rc.version = qualifiedSymbol.at(exclPos - 1) - '0';
        // Any namespace? 'QtCored4!nsp::qstrdup'
        const std::string::size_type nameSpaceStart = exclPos + 1;
        const std::string::size_type colonPos = qualifiedSymbol.find(':', nameSpaceStart);
        if (colonPos != std::string::npos)
            rc.nameSpace = qualifiedSymbol.substr(nameSpaceStart, colonPos - nameSpaceStart);

    } while (false);
    rc.qObjectType = rc.prependQtCoreModule("QObject");
    rc.qObjectPrivateType = rc.prependQtCoreModule("QObjectPrivate");
    rc.qWindowPrivateType = rc.prependQtGuiModule("QWindowPrivate");
    rc.qWidgetPrivateType =
        rc.prependQtModule("QWidgetPrivate", rc.version >= 5 ? Widgets : Gui);
    if (SymbolGroupValue::verbose)
        DebugPrint() << rc;
    return rc;
}

std::string QtInfo::moduleName(Module m) const
{
    // Must match the enumeration
    static const char* modNames[] =
        {"Core", "Gui", "Widgets", "Network", "Script" };
    std::ostringstream result;
    result << "Qt";
    if (version >= 5)
        result << version;
    result << modNames[m] << libInfix;
    return result.str();
}

std::string QtInfo::prependModuleAndNameSpace(const std::string &type,
                                              const std::string &module,
                                              const std::string &aNameSpace)
{
    // Strip the prefixes "class ", "struct ".
    std::string rc = SymbolGroupValue::stripClassPrefixes(type);
    // Is the namespace 'nsp::' missing?
    if (!aNameSpace.empty()) {
        const bool nameSpaceMissing = rc.size() <= aNameSpace.size()
                || rc.compare(0, aNameSpace.size(), aNameSpace) != 0
                || rc.at(aNameSpace.size()) != ':';
        if (nameSpaceMissing) {
            rc.insert(0, "::");
            rc.insert(0, aNameSpace);
        }
    }
    // Is the module 'Foo!' missing?
    if (!module.empty()) {
        const bool moduleMissing = rc.size() <= module.size()
                || rc.compare(0, module.size(), module) != 0
                || rc.at(module.size()) != '!';
        if (moduleMissing) {
            rc.insert(0, 1, '!');
            rc.insert(0, module);
        }
    }
    return rc;
}

std::ostream &operator<<(std::ostream &os, const QtInfo &i)
{
    os << "Qt Info: Version: " << i.version << " Modules '"
       << i.moduleName(QtInfo::Core) << "', '" << i.moduleName(QtInfo::Gui)
       << "', Namespace='" << i.nameSpace
       << "', types: " << i.qObjectType << ','
       << i.qObjectPrivateType << ',' << i.qWidgetPrivateType;
    return os;
}

std::list<std::string>
    SymbolGroupValue::resolveSymbolName(const char *pattern,
                                    const SymbolGroupValueContext &c,
                                    std::string *errorMessage /* = 0 */)
{
    // Extract the names
    const SymbolList symbols = resolveSymbol(pattern, c, errorMessage);
    std::list<std::string> rc;
    if (!symbols.empty()) {
        const SymbolList::const_iterator cend = symbols.end();
        for (SymbolList::const_iterator it = symbols.begin(); it != cend; ++it)
            rc.push_back(it->first);
    }
    return rc;

}

SymbolGroupValue::SymbolList
    SymbolGroupValue::resolveSymbol(const char *pattern,
                                    const SymbolGroupValueContext &c,
                                    std::string *errorMessage /* = 0 */)
{
    enum { bufSize = 2048 };
    std::list<Symbol> rc;
    if (errorMessage)
        errorMessage->clear();
    // Is it an incomplete symbol?
    if (!pattern[0])
        return rc;

    ULONG64 handle = 0;
    // E_NOINTERFACE means "no match". Apparently, it does not always
    // set handle.
    HRESULT hr = c.symbols->StartSymbolMatch(pattern, &handle);
    if (hr == E_NOINTERFACE) {
        if (handle)
            c.symbols->EndSymbolMatch(handle);
        return rc;
    }
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage= msgDebugEngineComFailed("StartSymbolMatch", hr);
        return rc;
    }
    char buf[bufSize];
    ULONG64 offset;
    while (true) {
        hr = c.symbols->GetNextSymbolMatch(handle, buf, bufSize - 1, 0, &offset);
        if (hr == E_NOINTERFACE)
            break;
        if (hr == S_OK)
            rc.push_back(Symbol(std::string(buf), offset));
    }
    c.symbols->EndSymbolMatch(handle);
    return rc;
}

// Resolve a type, that is, obtain its module name ('QString'->'QtCored4!QString')
std::string SymbolGroupValue::resolveType(const std::string &typeIn,
                                          const SymbolGroupValueContext &ctx,
                                          const std::string &currentModule /* = "" */)
{
    enum { BufSize = 512 };

    if (typeIn.empty() || typeIn.find('!') != std::string::npos)
        return typeIn;

    const std::string stripped = SymbolGroupValue::stripClassPrefixes(typeIn);

    // Use the module of the current symbol group for templates.
    // This is because resolving some template types (std::list<> has been
    // observed to result in 'QtGui4d!std::list', which subsequently fails.
    if (!currentModule.empty() && stripped.find('<') != std::string::npos) {
        std::string trc = currentModule;
        trc.push_back('!');
        trc += stripped;
        return trc;
    }
    // Obtain the base address of the module using an obscure ioctl() call.
    // See inline implementation of GetTypeSize() and docs.
    SYM_DUMP_PARAM symParameters = { sizeof (SYM_DUMP_PARAM), (PUCHAR)stripped.c_str(), DBG_DUMP_NO_PRINT, 0,
                                     NULL, NULL, NULL, 0, NULL };
    const ULONG typeSize = Ioctl(IG_GET_TYPE_SIZE, &symParameters, symParameters.size);
    if (!typeSize || !symParameters.ModBase) // Failed?
        return stripped;
    const std::string module = moduleNameByOffset(ctx.symbols, symParameters.ModBase);
    if (module.empty())
        return stripped;
    std::string rc = module;
    rc.push_back('!');
    rc += stripped;
    return rc;
}

// get the inner types: "QMap<int, double>" -> "int", "double"
std::vector<std::string> SymbolGroupValue::innerTypesOf(const std::string &t)
{
    std::vector<std::string> rc;

    std::string::size_type pos = t.find('<');
    if (pos == std::string::npos)
        return rc;

    rc.reserve(5);
    const std::string::size_type size = t.size();
    // Record all elements of level 1 to work correctly for
    // 'std::map<std::basic_string< .. > >'
    unsigned level = 0;
    std::string::size_type start = 0;
    for ( ; pos < size ; pos++) {
        const char c = t.at(pos);
        switch (c) {
        case '<':
            if (++level == 1)
                start = pos + 1;
            break;
        case '>':
            if (--level == 0) { // last element
                std::string innerType = t.substr(start, pos - start);
                trimFront(innerType);
                trimBack(innerType);
                rc.push_back(innerType);
                return rc;
            }
            break;
        case ',':
            if (level == 1) { // std::map<a, b>: start anew at ','.
                std::string innerType = t.substr(start, pos - start);
                trimFront(innerType);
                trimBack(innerType);
                rc.push_back(innerType);
                start = pos + 1;
            }
            break;
        }
    }
    return rc;
}

std::ostream &operator<<(std::ostream &str, const SymbolGroupValue &v)
{
    if (v) {
        str << '\'' << v.name() << "' 0x" << std::hex << v.address() <<
               std::dec << ' ' << v.type() << ": '" << wStringToString(v.value()) << '\'';
    } else {
        str << "Invalid value '" << v.error() << '\'';
    }
    return str;
}

// -------------------- Simple dumping helpers

// Courtesy of qdatetime.cpp
static inline void getDateFromJulianDay(unsigned julianDay, int *year, int *month, int *day)
{
    int y, m, d;

    if (julianDay >= 2299161) {
        typedef unsigned long long qulonglong;
        // Gregorian calendar starting from October 15, 1582
        // This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
        qulonglong ell, n, i, j;
        ell = qulonglong(julianDay) + 68569;
        n = (4 * ell) / 146097;
        ell = ell - (146097 * n + 3) / 4;
        i = (4000 * (ell + 1)) / 1461001;
        ell = ell - (1461 * i) / 4 + 31;
        j = (80 * ell) / 2447;
        d = int(ell - (2447 * j) / 80);
        ell = j / 11;
        m = int(j + 2 - (12 * ell));
        y = int(100 * (n - 49) + i + ell);
    } else {
        // Julian calendar until October 4, 1582
        // Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
        julianDay += 32082;
        int dd = (4 * julianDay + 3) / 1461;
        int ee = julianDay - (1461 * dd) / 4;
        int mm = ((5 * ee) + 2) / 153;
        d = ee - (153 * mm + 2) / 5 + 1;
        m = mm + 3 - 12 * (mm / 10);
        y = dd - 4800 + (mm / 10);
        if (y <= 0)
            --y;
    }
    if (year)
        *year = y;
    if (month)
        *month = m;
    if (day)
        *day = d;
}

// Convert and format Julian Date as used in QDate
static inline void formatJulianDate(std::wostream &str, unsigned julianDate)
{
    int y, m, d;
    getDateFromJulianDay(julianDate, &y, &m, &d);
    str << d << '.' << m << '.' << y;
}

// Format time in milliseconds as "hh:dd:ss:mmm"
static inline void formatMilliSeconds(std::wostream &str, int milliSecs)
{
    const int hourFactor = 1000 * 3600;
    const int hours = milliSecs / hourFactor;
    milliSecs = milliSecs % hourFactor;
    const int minFactor = 1000 * 60;
    const int minutes = milliSecs / minFactor;
    milliSecs = milliSecs % minFactor;
    const int secs = milliSecs / 1000;
    milliSecs = milliSecs % 1000;
    str.fill('0');
    str << std::setw(2) << hours << ':' << std::setw(2)
        << minutes << ':' << std::setw(2) << secs
        << '.' << std::setw(3) << milliSecs;
}

const char *stdStringTypeC = "std::basic_string<char,std::char_traits<char>,std::allocator<char> >";
const char *stdWStringTypeC = "std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >";
// Compiler option:  -Zc:wchar_t-:
const char *stdWStringWCharTypeC = "std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >";

static KnownType knownPODTypeHelper(const std::string &type, std::string::size_type endPos)
{
    if (type.empty() || !endPos)
        return KT_Unknown;
    // Strip pointer types.
    const bool isPointer = type.at(endPos - 1) == '*';
    if (isPointer) {
        endPos--;
        if (endPos > 0 && type.at(endPos - 1) == ' ')
            endPos--;
    }
    switch (type.at(0)) {
    case 'c':
        if (endPos == 4 && !type.compare(0, endPos, "char"))
            return isPointer ? KT_POD_PointerType : KT_Char;
        break;
    case 'd':
        if (endPos == 6 && !type.compare(0, endPos, "double"))
            return isPointer ? KT_POD_PointerType : KT_FloatType;
        break;
    case 'f':
        if (endPos == 5 && !type.compare(0, endPos, "float"))
            return isPointer ? KT_POD_PointerType : KT_FloatType;
        break;
    case 'l':
        if (endPos >= 4 && !type.compare(0, 4, "long"))
            if (endPos == 4 || type.at(4) == ' ')
                return isPointer ? KT_POD_PointerType : KT_IntType;
        break;
    case 'i':
        // 'int' 'int64'
        if (endPos >= 3 && !type.compare(0, 3, "int"))
            if (endPos == 3 || type.at(3) == ' ' || type.at(3) == '6')
                return isPointer ? KT_POD_PointerType : KT_IntType;
        break;
    case 's':
        if (endPos == 5 && !type.compare(0, 5, "short"))
            return isPointer ? KT_POD_PointerType : KT_IntType;
        if (endPos >= 6 && !type.compare(0, 6, "signed"))
            if (endPos == 6 || type.at(6) == ' ')
                return isPointer ? KT_POD_PointerType : KT_IntType;
        break;
    case 'u':
        if (endPos >= 8 && !type.compare(0, 8, "unsigned")) {
            if (endPos == 8 || type.at(8) == ' ') {
                if (isPointer)
                    return KT_POD_PointerType;
                return type.compare(0, 13, "unsigned char") ?
                            KT_UnsignedIntType :
                            KT_UnsignedChar;
            }
        }
        break;
    }
    return isPointer ? KT_PointerType : KT_Unknown;
}

// Determine type starting from a position (with/without 'class '/'struct ' prefix).
static KnownType knownClassTypeHelper(const std::string &type,
                                      std::string::size_type pos,
                                      std::string::size_type endPos)
{
    // STL ?
    const std::wstring::size_type templatePos = type.find('<', pos);
    static const std::wstring::size_type stlClassLen = 5;
    if (!type.compare(pos, stlClassLen, "std::")) {
        // STL containers
        const std::wstring::size_type hPos = pos + stlClassLen;
        if (templatePos != std::string::npos) {
            switch (templatePos - stlClassLen - pos) {
            case 3:
                if (!type.compare(hPos, 3, "set"))
                    return KT_StdSet;
                if (!type.compare(hPos, 3, "map"))
                    return KT_StdMap;
                break;
            case 4:
                if (!type.compare(hPos, 4, "list"))
                    return KT_StdList;
                break;
            case 5:
                if (!type.compare(hPos, 5, "stack"))
                    return KT_StdStack;
                if (!type.compare(hPos, 5, "deque"))
                    return KT_StdDeque;
                break;
            case 6:
                if (!type.compare(hPos, 6, "vector"))
                    return KT_StdVector;
                break;
            case 8:
                if (!type.compare(hPos, 8, "multimap"))
                    return KT_StdMultiMap;
                break;
            }
        }
        // STL strings
        if (!type.compare(pos, endPos - pos, stdStringTypeC))
            return KT_StdString;
        if (!type.compare(pos, endPos - pos, stdWStringTypeC)
            || !type.compare(pos, endPos - pos, stdWStringWCharTypeC))
            return KT_StdWString;
        return KT_Unknown;
    } // std::sth
    // Check for a 'Q' past the last namespace (beware of namespaced Qt:
    // 'nsp::QString').
    const std::wstring::size_type lastNameSpacePos = type.rfind(':', templatePos);
    const std::wstring::size_type qPos =
            lastNameSpacePos == std::string::npos ? type.find('Q', pos) : lastNameSpacePos + 1;
    if (qPos == std::string::npos || qPos >= type.size() || type.at(qPos) != 'Q')
        return KT_Unknown;
    // Qt types (templates)
    if (templatePos != std::string::npos) {
        // Do not fall for QMap<K,T>::iterator, which is actually an inner class.
        if (endPos > templatePos && type.at(endPos - 1) != '>')
            return KT_Unknown;
        switch (templatePos - qPos) {
        case 4:
            if (!type.compare(qPos, 4, "QSet"))
                return KT_QSet;
            if (!type.compare(qPos, 4, "QMap"))
                return KT_QMap;
            break;
        case 5:
            if (!type.compare(qPos, 5, "QHash"))
                return KT_QHash;
            if (!type.compare(qPos, 5, "QList"))
                return KT_QList;
            break;
        case 6:
            if (!type.compare(qPos, 6, "QFlags"))
                return KT_QFlags;
            if (!type.compare(qPos, 6, "QStack"))
                return KT_QStack;
            if (!type.compare(qPos, 6, "QQueue"))
                return KT_QQueue;
            break;
        case 7:
            if (!type.compare(qPos, 7, "QVector"))
                return KT_QVector;
            break;
        case 9:
            if (!type.compare(qPos, 9, "QMultiMap"))
                return KT_QMultiMap;
            break;
        case 10:
            if (!type.compare(qPos, 10, "QMultiHash"))
                return KT_QMultiHash;
            break;
        case 11:
            if (!type.compare(qPos, 11, "QLinkedList"))
                return KT_QLinkedList;
            break;
        case 14:
            if (!type.compare(qPos, 14, "QSharedPointer"))
                return KT_QSharedPointer;
            break;
        }
    }
    // Remaining non-template types
    switch (endPos - qPos) {
    case 4:
        if (!type.compare(qPos, 4, "QPen"))
            return KT_QPen;
        if (!type.compare(qPos, 4, "QUrl"))
            return KT_QUrl;
        if (!type.compare(qPos, 4, "QDir"))
            return KT_QDir;
        break;
    case 5:
        if (!type.compare(qPos, 5, "QChar"))
            return KT_QChar;
        if (!type.compare(qPos, 5, "QDate"))
            return KT_QDate;
        if (!type.compare(qPos, 5, "QTime"))
            return KT_QTime;
        if (!type.compare(qPos, 5, "QSize"))
            return KT_QSize;
        if (!type.compare(qPos, 5, "QLine"))
            return KT_QLine;
        if (!type.compare(qPos, 5, "QRect"))
            return KT_QRect;
        if (!type.compare(qPos, 5, "QIcon"))
            return KT_QIcon;
        if (!type.compare(qPos, 5, "QFile"))
            return KT_QFile;
        break;
    case 6:
        if (!type.compare(qPos, 6, "QColor"))
            return KT_QColor;
        if (!type.compare(qPos, 6, "QSizeF"))
            return KT_QSizeF;
        if (!type.compare(qPos, 6, "QPoint"))
            return KT_QPoint;
        if (!type.compare(qPos, 6, "QLineF"))
            return KT_QLineF;
        if (!type.compare(qPos, 6, "QRectF"))
            return KT_QRectF;
        if (!type.compare(qPos, 6, "QBrush"))
            return KT_QBrush;
        if (!type.compare(qPos, 6, "QImage"))
            return KT_QImage;
        if (!type.compare(qPos, 6, "QFixed"))
            return KT_QFixed;
        break;
    case 7:
        if (!type.compare(qPos, 7, "QString"))
            return KT_QString;
        if (!type.compare(qPos, 7, "QPointF"))
            return KT_QPointF;
        if (!type.compare(qPos, 7, "QObject"))
            return KT_QObject;
        if (!type.compare(qPos, 7, "QWidget"))
            return KT_QWidget;
        if (!type.compare(qPos, 7, "QWindow"))
            return KT_QWindow;
        if (!type.compare(qPos, 7, "QLocale"))
            return KT_QLocale;
        if (!type.compare(qPos, 7, "QMatrix"))
            return KT_QMatrix;
        if (!type.compare(qPos, 7, "QRegExp"))
            return KT_QRegExp;
        break;
    case 8:
        if (!type.compare(qPos, 8, "QVariant"))
            return KT_QVariant;
        if (!type.compare(qPos, 8, "QMargins"))
            return KT_QMargins;
        if (!type.compare(qPos, 8, "QXmlItem"))
            return KT_QXmltem;
        if (!type.compare(qPos, 8, "QXmlName"))
            return KT_QXmlName;
        if (!type.compare(qPos, 8, "QProcess"))
            return KT_QProcess;
        break;
    case 9:
        if (!type.compare(qPos, 9, "QBitArray"))
            return KT_QBitArray;
        if (!type.compare(qPos, 9, "QDateTime"))
            return KT_QDateTime;
        if (!type.compare(qPos, 9, "QFileInfo"))
            return KT_QFileInfo;
        if (!type.compare(qPos, 9, "QMetaEnum"))
            return KT_QMetaEnum;
        if (!type.compare(qPos, 9, "QTextItem"))
            return KT_QTextItem;
        if (!type.compare(qPos, 9, "QVector2D"))
            return KT_QVector2D;
        if (!type.compare(qPos, 9, "QVector3D"))
            return KT_QVector3D;
        if (!type.compare(qPos, 9, "QVector4D"))
            return KT_QVector4D;
        break;
    case 10:
        if (!type.compare(qPos, 10, "QAtomicInt"))
            return KT_QAtomicInt;
        if (!type.compare(qPos, 10, "QByteArray"))
            return KT_QByteArray;
        if (!type.compare(qPos, 10, "QMatrix4x4"))
            return KT_QMatrix4x4;
        if (!type.compare(qPos, 10, "QTextBlock"))
            return KT_QTextBlock;
        if (!type.compare(qPos, 10, "QTransform"))
            return KT_QTransform;
        if (!type.compare(qPos, 10, "QFixedSize"))
            return KT_QFixedSize;
        if (!type.compare(qPos, 10, "QStringRef"))
            return KT_QStringRef;
        break;
    case 11:
        if (!type.compare(qPos, 11, "QStringList"))
            return KT_QStringList;
        if (!type.compare(qPos, 11, "QBasicTimer"))
            return KT_QBasicTimer;
        if (!type.compare(qPos, 11, "QMetaMethod"))
            return KT_QMetaMethod;
        if (!type.compare(qPos, 11, "QModelIndex"))
            return KT_QModelIndex;
        if (!type.compare(qPos, 11, "QQuaternion"))
            return KT_QQuaternion;
        if (!type.compare(qPos, 11, "QScriptItem"))
            return KT_QScriptItem;
        if (!type.compare(qPos, 11, "QFixedPoint"))
            return KT_QFixedPoint;
        if (!type.compare(qPos, 11, "QScriptLine"))
            return KT_QScriptLine;
        break;
    case 12:
        if (!type.compare(qPos, 12, "QKeySequence"))
            return KT_QKeySequence;
        if (!type.compare(qPos, 12, "QHostAddress"))
            return KT_QHostAddress;
        if (!type.compare(qPos, 12, "QScriptValue"))
            return KT_QScriptValue;
        break;
    case 13:
        if (!type.compare(qPos, 13, "QTextFragment"))
            return KT_QTextFragment;
        if (!type.compare(qPos, 13, "QTreeViewItem"))
            return KT_QTreeViewItem;
        break;
    case 14:
        if (!type.compare(qPos, 14, "QMetaClassInfo"))
            return KT_QMetaClassInfo;
        if (!type.compare(qPos, 14, "QNetworkCookie"))
            return KT_QNetworkCookie;
        break;
    case 15:
        if (!type.compare(qPos, 15, "QBasicAtomicInt"))
            return KT_QBasicAtomicInt;
        if (!type.compare(qPos, 15, "QHashDummyValue"))
            return KT_QHashDummyValue;
        if (!type.compare(qPos, 15, "QSourceLocation"))
            return KT_QSourceLocation;
        if (!type.compare(qPos, 15, "QScriptAnalysis"))
            return KT_QScriptAnalysis;
        break;
    case 16:
        if (!type.compare(qPos, 16, "QTextUndoCommand"))
            return KT_QTextUndoCommand;
        break;
    case 18:
        if (!type.compare(qPos, 18, "QNetworkProxyQuery"))
            return KT_QNetworkProxyQuery;
        if (!type.compare(qPos, 18, "QXmlNodeModelIndex"))
            return KT_QXmlNodeModelIndex;
        break;
    case 19:
        if (!type.compare(qPos, 19, "QItemSelectionRange"))
            return KT_QItemSelectionRange;
        if (!type.compare(qPos, 19, "QPaintBufferCommand"))
            return KT_QPaintBufferCommand;
        if (!type.compare(qPos, 19, "QTextHtmlParserNode"))
            return KT_QTextHtmlParserNode;
        if (!type.compare(qPos, 19, "QXmlStreamAttribute"))
            return KT_QXmlStreamAttribute;
        if (!type.compare(qPos, 19, "QGlyphJustification"))
            return KT_QGlyphJustification;
        break;
    case 20:
        if (!type.compare(qPos, 20, "QTextBlock::iterator"))
            return KT_QTextBlock_iterator;
        if (!type.compare(qPos, 20, "QTextFrame::iterator"))
            return KT_QTextFrame_iterator;
        break;
    case 21:
        if (!type.compare(qPos, 21, "QPersistentModelIndex"))
            return KT_QPersistentModelIndex;
        if (!type.compare(qPos, 21, "QPainterPath::Element"))
            return KT_QPainterPath_Element;
        break;
    case 22:
        if (!type.compare(qPos, 22, "QObjectPrivate::Sender"))
            return KT_QObjectPrivate_Sender;
        break;
    case 24:
        if (!type.compare(qPos, 24, "QPatternist::AtomicValue"))
            return KT_QPatternist_AtomicValue;
        if (!type.compare(qPos, 24, "QPatternist::Cardinality"))
            return KT_QPatternist_Cardinality;
        break;
    case 26:
        if (!type.compare(qPos, 26, "QObjectPrivate::Connection"))
            return KT_QObjectPrivate_Connection;
        if (!type.compare(qPos, 26, "QPatternist::ItemCacheCell"))
            return KT_QPatternist_ItemCacheCell;
        if (!type.compare(qPos, 26, "QPatternist::ItemType::Ptr"))
            return KT_QPatternist_ItemType_Ptr;
        if (!type.compare(qPos, 26, "QPatternist::NamePool::Ptr"))
            return KT_QPatternist_NamePool_Ptr;
        break;
    case 27:
        if (!type.compare(qPos, 27, "QXmlStreamEntityDeclaration"))
            return KT_QXmlStreamEntityDeclaration;
        break;
    case 28:
        if (!type.compare(qPos, 28, "QPatternist::Expression::Ptr"))
            return KT_QPatternist_Expression_Ptr;
        break;
    case 29:
        if (!type.compare(qPos, 29, "QXmlStreamNotationDeclaration"))
            return KT_QXmlStreamNotationDeclaration;
    case 30:
        if (!type.compare(qPos, 30, "QPatternist::SequenceType::Ptr"))
            return KT_QPatternist_SequenceType_Ptr;
        if (!type.compare(qPos, 30, "QXmlStreamNamespaceDeclaration"))
            return KT_QXmlStreamNamespaceDeclaration;
        break;
    case 32:
        break;
        if (!type.compare(qPos, 32, "QPatternist::Item::Iterator::Ptr"))
            return KT_QPatternist_Item_Iterator_Ptr;
    case 34:
        break;
        if (!type.compare(qPos, 34, "QPatternist::ItemSequenceCacheCell"))
            return KT_QPatternist_ItemSequenceCacheCell;
    case 37:
        break;
        if (!type.compare(qPos, 37, "QNetworkHeadersPrivate::RawHeaderPair"))
            return KT_QNetworkHeadersPrivate_RawHeaderPair;
        if (!type.compare(qPos, 37, "QPatternist::AccelTree::BasicNodeData"))
            return KT_QPatternist_AccelTree_BasicNodeData;
        break;
    }
    return KT_Unknown;
}

KnownType knownType(const std::string &type, unsigned flags)
{
    if (type.empty())
        return KT_Unknown;
    // Autostrip one pointer if desired
    const std::string::size_type endPos = (flags & KnownTypeAutoStripPointer) ?
                type.size() - SymbolGroupValue::isPointerType(type) :
                type.size();

    // PODs first
    const KnownType podType = knownPODTypeHelper(type, endPos);
    if (podType != KT_Unknown)
        return podType;

    if (flags & KnownTypeHasClassPrefix) {
        switch (type.at(0)) { // Check 'class X' or 'struct X'
        case 'c':
            if (!type.compare(0, 6, "class "))
                return knownClassTypeHelper(type, 6, endPos);
            break;
        case 's':
            if (!type.compare(0, 7, "struct "))
                return knownClassTypeHelper(type, 7, endPos);
            break;
        }
    } else {
        // No prefix, full check
        return knownClassTypeHelper(type, 0, endPos);
    }
    return KT_Unknown;
}

void formatKnownTypeFlags(std::ostream &os, KnownType kt)
{
    switch (kt) {
    case KT_Unknown:
        os << "<unknown>";
        return;
    case KT_POD_PointerType:
        os << " pod_pointer";
        break;
    case KT_PointerType:
        os << " pointer";
        break;
    default:
        break;
    }
    if (kt & KT_POD_Type)
        os << " pod";
    if (kt & KT_Qt_Type)
        os << " qt";
    if (kt & KT_Qt_PrimitiveType)
        os << " qt_primitive";
    if (kt & KT_Qt_MovableType)
        os << " qt_movable";
    if (kt & KT_ContainerType)
        os << " container";
    if (kt & KT_STL_Type)
        os << " stl";
    if (kt & KT_HasSimpleDumper)
        os << " simple_dumper";
}

static inline DumpParameterRecodeResult
    checkCharArrayRecode(const SymbolGroupValue &v)
{
    return DumpParameters::checkRecode(v.type(), std::string(),
                                       v.value(), v.context(), v.address());
}

// Helper struct containing data Address and size/alloc information
// from Qt's QString/QByteArray.
struct QtStringAddressData
{
    QtStringAddressData() : address(0), size(0), allocated(0) {}

    ULONG64 address;
    unsigned size; // Size and allocated are in ushort for QString's.
    unsigned allocated;
};

/* Helper to determine the location and size of the data of
 * QStrings/QByteArrays for versions 4,5. In Qt 4, 'd' has a 'data'
 * pointer. In Qt 5, the d-elements and the data are in a storage pool
 * and the data are at an offset behind the d-structures (QString,
 * QByteArray, QVector). */
QtStringAddressData readQtStringAddressData(const SymbolGroupValue &dV,
                                           int qtMajorVersion)
{
    QtStringAddressData result;
    const SymbolGroupValue sizeV = dV["size"];
    const SymbolGroupValue allocV = dV["alloc"];
    if (!sizeV || !allocV)
        return QtStringAddressData();
    result.size = sizeV.intValue();
    result.allocated = allocV.intValue();
    if (qtMajorVersion < 5) {
        // Qt 4: Simple 'data' pointer.
        result.address = dV["data"].pointerValue();
    } else {
        // Qt 5: Memory pool after the data element.
        const SymbolGroupValue offsetV = dV["offset"];
        if (!offsetV)
            return QtStringAddressData();
        // Take the address for QTypeArrayData of QByteArray, else
        // pointer value of D-pointer.
        const ULONG64 baseAddress = SymbolGroupValue::isPointerType(dV.type()) ?
                                    dV.pointerValue() : dV.address();
        result.address = baseAddress + offsetV.intValue();
    }
    return result;
}

// Retrieve data from a QByteArrayData(char)/QStringData(wchar_t)
// in desired type. For empty arrays, no data are allocated.
// All sizes are in CharType units. zeroTerminated means data are 0-terminated
// in the data type, but "size" does not contain it.
template <typename CharType>
bool readQt5StringData(const SymbolGroupValue &dV, int qtMajorVersion,
                       bool zeroTerminated, unsigned position, unsigned sizeLimit,
                       unsigned *fullSize, unsigned *arraySize,
                       CharType **array)
{
    *array = 0;
    const QtStringAddressData data = readQtStringAddressData(dV, qtMajorVersion);
    if (!data.address || position > data.size)
        return false;
    const ULONG64 address = data.address + sizeof(CharType) * position;
    *fullSize = data.size - position;
    *arraySize = std::min(*fullSize, sizeLimit);
    if (!*fullSize)
        return true;
    const unsigned memorySize =
            sizeof(CharType) * (*arraySize + (zeroTerminated ? 1 : 0));
    unsigned char *memory =
            SymbolGroupValue::readMemory(dV.context().dataspaces,
                                         address, memorySize);
    if (!memory)
        return false;
    *array = reinterpret_cast<CharType *>(memory);
    if ((*arraySize < *fullSize) && zeroTerminated)
        *(*array + *arraySize) = CharType(0);
    return true;
}

static inline bool dumpQString(const SymbolGroupValue &v, std::wostream &str,
                               MemoryHandle **memoryHandle = 0,
                               unsigned position = 0,
                               unsigned length = std::numeric_limits<unsigned>::max())
{
    const QtInfo &qtInfo = QtInfo::get(v.context());
    const SymbolGroupValue dV = v["d"];
    if (!dV)
        return false;
    // Qt 4.
    if (qtInfo.version < 5) {
        if (const SymbolGroupValue sizeValue = dV["size"]) {
            const int size = sizeValue.intValue();
            if (size >= 0) {
                std::wstring stringData = dV["data"].wcharPointerData(size);
                if (position && position < stringData.size())
                    stringData.erase(0, position);
                if (length < stringData.size())
                    stringData.erase(length, stringData.size() - length);
                str << L'"' << stringData << L'"';
                if (memoryHandle)
                    *memoryHandle = MemoryHandle::fromStdWString(stringData);
                return true;
            }
        }
        return false;

    } // Qt 4.

    wchar_t *memory;
    unsigned fullSize;
    unsigned size;
    const SymbolGroupValue typeArrayV = dV[unsigned(0)];
    if (!typeArrayV)
        return false;
    if (!readQt5StringData(typeArrayV, qtInfo.version, true, position,
                           std::min(length, ExtensionContext::instance().parameters().maxStringLength),
                           &fullSize, &size, &memory))
        return false;
    if (size) {
        str << L'"' << memory;
        if (std::min(fullSize, length) > size)
            str << L"...";
        str << L'"';
    } else {
        str << L"\"\"";
    }
    if (memoryHandle)
        *memoryHandle = new MemoryHandle(memory, size);
    else
        delete [] memory;
    return true;
}

static inline bool dumpQStringRef(const SymbolGroupValue &v, std::wostream &str,
                                  MemoryHandle **memoryHandle = 0)
{
    const int position = v["m_position"].intValue();
    const int size = v["m_size"].intValue();
    if (position < 0 || size < 0)
        return false;
    const SymbolGroupValue string = v["m_string"];
    if (!string || !dumpQString(string, str, memoryHandle, position, size))
        return false;
    str << L" (" << position << ',' << size << L')';
    return true;
}

/* Pad a memory offset to align with pointer size */
static inline unsigned padOffset(unsigned offset)
{
    const unsigned ps = SymbolGroupValue::pointerSize();
    return (offset % ps) ? (1 + offset / ps) * ps : offset;
}

/* Return the offset to be accounted for "QSharedData" to access
 * the first member of a QSharedData-derived class */
static unsigned qSharedDataOffset(const SymbolGroupValueContext &ctx)
{
    unsigned offset = 0;
    if (!offset) {
        // As of 4.X, a QAtomicInt, which will be padded to 8 on a 64bit system.
        const std::string qSharedData = QtInfo::get(ctx).prependQtCoreModule("QSharedData");
        offset = padOffset(SymbolGroupValue::sizeOf(qSharedData.c_str()));
    }
    return offset;
}

/* Return the size of a QString */
static unsigned qStringSize(const SymbolGroupValueContext &ctx)
{
    static const unsigned size = SymbolGroupValue::sizeOf(QtInfo::get(ctx).prependQtCoreModule("QString").c_str());
    return size;
}

/* Return the size of a QList via QStringList. */
static unsigned qListSize(const SymbolGroupValueContext &ctx)
{
    static const unsigned size = SymbolGroupValue::sizeOf(QtInfo::get(ctx).prependQtCoreModule("QStringList").c_str());
    return size;
}

/* Return the size of a QByteArray */
static unsigned qByteArraySize(const SymbolGroupValueContext &ctx)
{
    static const unsigned size = SymbolGroupValue::sizeOf(QtInfo::get(ctx).prependQtCoreModule("QByteArray").c_str());
    return size;
}

/* Return the size of a QAtomicInt */
static unsigned qAtomicIntSize(const SymbolGroupValueContext &ctx)
{
    static const unsigned size = SymbolGroupValue::sizeOf(QtInfo::get(ctx).prependQtCoreModule("QAtomicInt").c_str());
    return size;
}

// Dump a QByteArray
static inline bool dumpQByteArray(const SymbolGroupValue &v, std::wostream &str,
                                  MemoryHandle **memoryHandle = 0)
{
    const QtInfo &qtInfo = QtInfo::get(v.context());
    const SymbolGroupValue dV = v["d"];
    if (!dV)
        return false;
    // Qt 4.
    if (qtInfo.version < 5) {
        if (const SymbolGroupValue data = dV["data"]) {
            const DumpParameterRecodeResult check =
                checkCharArrayRecode(data);
            if (check.buffer) {
                str << quotedWStringFromCharData(check.buffer, check.size);
                delete [] check.buffer;
            } else {
                str << data.value();
            }
            return true;
        }
        return false;
    }

    // Qt 5: Data start at offset past the 'd' of type QByteArrayData.
    char *memory;
    unsigned fullSize;
    unsigned size;
    const SymbolGroupValue typeArrayV = dV[unsigned(0)];
    if (!typeArrayV)
        return false;
    if (!readQt5StringData(typeArrayV, qtInfo.version, false, 0, 10240, &fullSize, &size, &memory))
        return false;
    if (size) {
        // Emulate CDB's behavior of replacing unprintable characters
        // by dots.
        std::wstring display;
        display.reserve(size);
        char *memEnd = memory + size;
        for (char *p = memory; p < memEnd; p++) {
            const char c = *p;
            display.push_back(c >= 0 && isprint(c) ? wchar_t(c) : L'.');
        }
        str << fullSize << L" bytes \"" << display;
        if (fullSize > size)
            str << L"...";
        str << L'"';
    } else {
        str << L"<empty>";
    }
    if (memoryHandle)
        *memoryHandle = new MemoryHandle(reinterpret_cast<unsigned char *>(memory), size);
    else
        delete [] memory;
    return true;
}

/* Below are some helpers for simple dumpers for some Qt classes accessing their
 * private classes without the debugger's symbolic information (applicable to non-exported
 * private classes such as QFileInfoPrivate, etc). This is done by dereferencing the
 * d-ptr and obtaining the address of the variable (considering some offsets depending on type)
 * and adding a symbol for that QString or QByteArray (basically using raw memory).
 */

enum QPrivateDumpMode // Enumeration determining the offsets to be taken into consideration
{
    QPDM_None,
    QPDM_qVirtual, // For classes with virtual functions (QObject-based): Skip vtable for d-address
    QPDM_qSharedData // Private class is based on QSharedData.
};

// Determine the address of private class member by dereferencing the d-ptr and using offsets.
static ULONG64 addressOfQPrivateMember(const SymbolGroupValue &v, QPrivateDumpMode mode,
                                       unsigned additionalOffset = 0)
{
    std::string errorMessage;
    // Dererence d-Ptr (Pointer/Value duality: Value or object address).
    ULONG64 dAddress = SymbolGroupValue::isPointerType(v.type()) ? v.pointerValue() : v.address();
    if (!dAddress)
        return 0;
    if (mode == QPDM_qVirtual) // Skip vtable.
        dAddress += SymbolGroupValue::pointerSize();
    const ULONG64 dptr = SymbolGroupValue::readPointerValue(v.context().dataspaces,
                                                            dAddress, &errorMessage);
    if (!dptr)
        return 0;
    // Get address of type to be dumped.
    ULONG64 dumpAddress = dptr  + additionalOffset;
    if (mode == QPDM_qSharedData) // Based on QSharedData
        dumpAddress += qSharedDataOffset(v.context());
    return dumpAddress;
}

// Convenience to dump a QString from the unexported private class of a Qt class.
static bool dumpQStringFromQPrivateClass(const SymbolGroupValue &v,
                                         QPrivateDumpMode mode,
                                         unsigned additionalOffset,
                                         std::wostream &str)
{
    std::string errorMessage;
    const ULONG64 stringAddress = addressOfQPrivateMember(v, mode, additionalOffset);
    if (!stringAddress)
        return false;
    const std::string dumpType = QtInfo::get(v.context()).prependQtCoreModule("QString");
    const std::string symbolName = SymbolGroupValue::pointedToSymbolName(stringAddress , dumpType);
    if (SymbolGroupValue::verbose > 1)
        DebugPrint() <<  "dumpQStringFromQPrivateClass of " << v.name() << '/'
                     << v.type() << " mode=" << mode
                     << " offset=" << additionalOffset << " address=0x" << std::hex << stringAddress
                     << std::dec << " expr=" << symbolName;
    SymbolGroupNode *stringNode =
            v.node()->symbolGroup()->addSymbol(v.module(), symbolName, std::string(), &errorMessage);
    if (!stringNode)
        return false;
    return dumpQString(SymbolGroupValue(stringNode, v.context()), str);
}

// Convenience to dump a QByteArray from the unexported private class of a Qt class.
static bool dumpQByteArrayFromQPrivateClass(const SymbolGroupValue &v,
                                            QPrivateDumpMode mode,
                                            unsigned additionalOffset,
                                            std::wostream &str)
{
    std::string errorMessage;
    const ULONG64 byteArrayAddress = addressOfQPrivateMember(v, mode, additionalOffset);
    if (!byteArrayAddress)
        return false;
    const std::string dumpType = QtInfo::get(v.context()).prependQtCoreModule("QByteArray");
    const std::string symbolName = SymbolGroupValue::pointedToSymbolName(byteArrayAddress , dumpType);
    SymbolGroupNode *byteArrayNode =
            v.node()->symbolGroup()->addSymbol(v.module(), symbolName, std::string(), &errorMessage);
    if (!byteArrayNode)
        return false;
    return dumpQByteArray(SymbolGroupValue(byteArrayNode, v.context()), str);
}

/* Dump QFileInfo, for whose private class no debugging information is available.
 * Dump 2nd string past its QSharedData base class. */
static inline bool dumpQFileInfo(const SymbolGroupValue &v, std::wostream &str)
{
    return dumpQStringFromQPrivateClass(v, QPDM_qSharedData, qStringSize(v.context()),  str);
}

/* Dump QDir, for whose private class no debugging information is available.
 * Dump 1st string past its QSharedData base class. */
static bool inline dumpQDir(const SymbolGroupValue &v, std::wostream &str)
{
    // Access QDirPrivate's dirEntry, which has the path as first member.
    const unsigned listSize = qListSize(v.context());
    const unsigned offset = padOffset(listSize + 2 * SymbolGroupValue::intSize())
            + padOffset(SymbolGroupValue::pointerSize() + SymbolGroupValue::sizeOf("bool"))
            + 2 * listSize;
    return dumpQStringFromQPrivateClass(v, QPDM_qSharedData, offset,  str);
}

/* Dump QRegExp, for whose private class no debugging information is available.
 * Dump 1st string past of its base class. */
static inline bool dumpQRegExp(const SymbolGroupValue &v, std::wostream &str)
{
    return dumpQStringFromQPrivateClass(v, QPDM_qSharedData, 0,  str);
}

/* Dump QFile, for whose private class no debugging information is available.
 * Dump the 1st string first past its QIODevicePrivate base class. */
static inline bool dumpQFile(const SymbolGroupValue &v, std::wostream &str)
{
    // Get address of the file name string, obtain value by dumping a QString at address
    static unsigned qIoDevicePrivateSize = 0;
    if (!qIoDevicePrivateSize) {
        const std::string qIoDevicePrivateType = QtInfo::get(v.context()).prependQtCoreModule("QIODevicePrivate");
        qIoDevicePrivateSize = padOffset(SymbolGroupValue::sizeOf(qIoDevicePrivateType.c_str()));
    }
    if (!qIoDevicePrivateSize)
        return false;
    return dumpQStringFromQPrivateClass(v, QPDM_qVirtual, qIoDevicePrivateSize,  str);
}

/* Dump QHostAddress, for whose private class no debugging information is available.
 * Dump string 'ipString' past of its private class. Does not currently work? */
static inline bool dumpQHostAddress(const SymbolGroupValue &v, std::wostream &str)
{
    // Determine offset in private struct: qIPv6AddressType (array, unaligned) +  uint32 + enum.
    static unsigned qIPv6AddressSize = 0;
    if (!qIPv6AddressSize) {
        const std::string qIPv6AddressType = QtInfo::get(v.context()).prependQtNetworkModule("QIPv6Address");
        qIPv6AddressSize = SymbolGroupValue::sizeOf(qIPv6AddressType.c_str());
    }
    if (!qIPv6AddressSize)
        return false;
    const unsigned offset = padOffset(8 + qIPv6AddressSize);
    return dumpQStringFromQPrivateClass(v, QPDM_None, offset,  str);
}

/* Dump QProcess, for whose private class no debugging information is available.
 * Dump string 'program' string with empirical offset. */
static inline bool dumpQProcess(const SymbolGroupValue &v, std::wostream &str)
{
    const unsigned offset = SymbolGroupValue::pointerSize() == 8 ? 424 : 260;
    return dumpQStringFromQPrivateClass(v, QPDM_qVirtual, offset,  str);
}

/* Dump QScriptValue, for whose private class no debugging information is available.
 * Private class has a pointer to engine, type enumeration and a JSC:JValue and double/QString
 * for respective types. */
static inline bool dumpQScriptValue(const SymbolGroupValue &v, std::wostream &str)
{
    std::string errorMessage;
    // Read out type
    const ULONG64 privateAddress = addressOfQPrivateMember(v, QPDM_None, 0);
    if (!privateAddress) { // Can actually be 0 for default-constructed
        str << L"<Invalid>";
        return true;
    }
    const unsigned ps = SymbolGroupValue::pointerSize();
    // Offsets of QScriptValuePrivate
    const unsigned jscScriptValueSize = 8; // Union of double and rest.
    const unsigned doubleValueOffset = 2 * ps + jscScriptValueSize;
    const unsigned stringValueOffset = doubleValueOffset + sizeof(double);
    const ULONG64 type =
        SymbolGroupValue::readUnsignedValue(v.context().dataspaces,
                                            privateAddress + ps, 4, 0, &errorMessage);
    switch (type) {
    case 1:
        str << SymbolGroupValue::readDouble(v.context().dataspaces, privateAddress + doubleValueOffset);
        break;
    case 2:
        return dumpQStringFromQPrivateClass(v, QPDM_None, stringValueOffset, str);
    default:
        str << L"<JavaScriptCore>";
        break;
    }
    return true;
}

/* Dump QUrl for whose private class no debugging information is available.
 * Dump the 'originally encoded' byte array of its private class. */
static inline bool dumpQUrl(const SymbolGroupValue &v, std::wostream &str)
{
    // Get address of the original-encoded byte array, obtain value by dumping at address
    if (QtInfo::get(v.context()).version < 5) {
        const ULONG offset = padOffset(qAtomicIntSize(v.context()))
                         + 6 * qStringSize(v.context()) + qByteArraySize(v.context());
        return dumpQByteArrayFromQPrivateClass(v, QPDM_None, offset, str);
    }
    ULONG offset = qAtomicIntSize(v.context()) +
                   SymbolGroupValue::intSize();
    const unsigned stringSize = qStringSize(v.context());
    str << L"Scheme: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    offset += stringSize;
    str << L" User: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    offset += stringSize;
    str << L" Password: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    offset += stringSize;
    str << L" Host: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    offset += stringSize;
    str << L" Path: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    offset += stringSize;
    str << L" Query: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    offset += stringSize;
    str << L" Fragment: ";
    if (!dumpQStringFromQPrivateClass(v, QPDM_None, offset, str))
        return false;
    return true;
}

// Dump QColor
static bool dumpQColor(const SymbolGroupValue &v, std::wostream &str)
{
    const SymbolGroupValue specV = v["cspec"];
    if (!specV)
        return false;
    const int spec = specV.intValue();
    if (spec == 0) {
        str << L"<Invalid color>";
        return true;
    }
    if (spec < 1 || spec > 4)
        return false;
    const SymbolGroupValue arrayV = v["ct"]["array"];
    if (!arrayV)
        return false;
    const int a0 = arrayV["0"].intValue();
    const int a1 = arrayV["1"].intValue();
    const int a2 = arrayV["2"].intValue();
    const int a3 = arrayV["3"].intValue();
    const int a4 = arrayV["4"].intValue();
    if (a0 < 0 || a1 < 0 || a2 < 0 || a3 < 0 || a4 < 0)
        return false;
    switch (spec) {
    case 1: // Rgb
        str << L"RGB alpha=" << (a0 / 0x101) << L", red=" << (a1 / 0x101)
            << L", green=" << (a2 / 0x101) << ", blue=" << (a3 / 0x101);
        break;
    case 2: // Hsv
        str << L"HSV alpha=" << (a0 / 0x101) << L", hue=" << (a1 / 100)
            << L", sat=" << (a2 / 0x101) << ", value=" << (a3 / 0x101);
        break;
    case 3: // Cmyk
        str << L"CMYK alpha=" << (a0 / 0x101) << L", cyan=" << (a1 / 100)
            << L", magenta=" << (a2 / 0x101) << ", yellow=" << (a3 / 0x101)
            << ", black=" << (a4 / 0x101);
        break;
    case 4: // Hsl
        str << L"HSL alpha=" << (a0 / 0x101) << L", hue=" << (a1 / 100)
            << L", sat=" << (a2 / 0x101) << ", lightness=" << (a3 / 0x101);
        break;
    }
    return true;
}

// Dump Qt's core types

static inline bool dumpQBasicAtomicInt(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue iValue = v["_q_value"]) {
        str <<  iValue.value();
        return true;
    }
    return false;
}

static inline bool dumpQAtomicInt(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue base = v[unsigned(0)])
        return dumpQBasicAtomicInt(base, str);
    return false;
}

static bool dumpQChar(const SymbolGroupValue &v, std::wostream &str)
{
    if (SymbolGroupValue cValue = v["ucs"]) {
        const int utf16 = cValue.intValue();
        if (utf16 >= 0) {
            // Print code = character,
            // exclude control characters and Pair indicator
            str << utf16;
            if (utf16 >= 32 && (utf16 < 0xD800 || utf16 > 0xDBFF))
                str << " '" << wchar_t(utf16) << '\'';
        }
        return true;
    }
    return false;
}

static inline bool dumpQFlags(const SymbolGroupValue &v, std::wostream &str)
{
    if (SymbolGroupValue iV = v["i"]) {
        const int i = iV.intValue();
        if (i >= 0) {
            str << i;
            return true;
        }
    }
    return false;
}

static bool dumpJulianDate(int julianDay, std::wostream &str)
{
    if (julianDay < 0)
        return false;
    else if (!julianDay)
        str << L"<null>";
    else
        formatJulianDate(str, julianDay);
    return true;
}

static bool dumpQDate(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue julianDayV = v["jd"])
        return dumpJulianDate(julianDayV.intValue(), str);
    return false;
}

static bool dumpQTime(const SymbolGroupValue &v, std::wostream &str)
{
    if (const SymbolGroupValue milliSecsV = v["mds"]) {
        const int milliSecs = milliSecsV.intValue();
        if (milliSecs >= 0) {
            formatMilliSeconds(str, milliSecs);
            return true;
        }
    }
    return false;
}

// QDateTime has an unexported private class. Obtain date and time
// from memory.
static bool dumpQDateTime(const SymbolGroupValue &v, std::wostream &str)
{
    const ULONG64 dateAddr = addressOfQPrivateMember(v, QPDM_qSharedData, 0);
    if (!dateAddr)
        return false;
    const int date =
        SymbolGroupValue::readIntValue(v.context().dataspaces,
                                       dateAddr, SymbolGroupValue::intSize(), 0);
    if (!date) {
        str << L"<null>";
        return true;
    }
    if (!dumpJulianDate(date, str))
        return false;
    const ULONG64 timeAddr = dateAddr + padOffset(SymbolGroupValue::intSize());
    const int time =
        SymbolGroupValue::readIntValue(v.context().dataspaces,
                                       timeAddr, SymbolGroupValue::intSize(), 0);
    str << L' ';
    formatMilliSeconds(str, time);
    return true;
}

static bool dumpQImage(const SymbolGroupValue &v, std::wostream &str, MemoryHandle **memoryHandle)
{
    struct CreatorImageHeader { // Header for image display as edit format, followed by data.
        int width;
        int height;
        int format;
    };
    const QtInfo &qtInfo(QtInfo::get(v.context()));
    // Fetch data of unexported private class
    const ULONG64 address = v["d"].pointerValue();
    if (!address) {
        str << L"<null>";
        return true;
    }
    const std::string qImageDataType = qtInfo.prependQtGuiModule("QImageData");
    const unsigned long size = SymbolGroupValue::sizeOf(qImageDataType.c_str());
    if (!size)
        return false;
    unsigned char *qImageData = SymbolGroupValue::readMemory(v.context().dataspaces, address, size);
    if (!qImageData)
        return false;
    // read size data
    unsigned char *ptr = qImageData + qAtomicIntSize(v.context());
    CreatorImageHeader header;
    header.width = *(reinterpret_cast<int *>(ptr));
    ptr += SymbolGroupValue::intSize();
    header.height = *(reinterpret_cast<int *>(ptr));
    ptr += SymbolGroupValue::intSize();
    const int depth = *(reinterpret_cast<int *>(ptr));
    ptr += SymbolGroupValue::intSize();
    const int nbytes = *(reinterpret_cast<int *>(ptr));
    const unsigned dataOffset = SymbolGroupValue::fieldOffset(qImageDataType.c_str(), "data");
    // Qt 4 has a Qt 3 support pointer member between 'data' and 'format'.
    const unsigned formatOffset = SymbolGroupValue::fieldOffset(qImageDataType.c_str(), "format");
    if (!dataOffset || !formatOffset)
        return false;
    ptr = qImageData + dataOffset;
    // read data pointer
    ULONG64 data = 0;
    memcpy(&data, ptr, SymbolGroupValue::pointerSize());
    // read format
    ptr = qImageData + formatOffset;
    header.format = *(reinterpret_cast<int *>(ptr));
    if (header.width < 0 || header.height < 0 || header.format < 0 || header.format > 255
        || nbytes < 0 || depth < 0) {
        return false;
    }
    str << header.width << L'x' << header.height << L", depth: " << depth
        << L", format: " << header.format << L", "
        << nbytes << L" bytes @0x"  << std::hex << data << std::dec;
    delete [] qImageData;
    // Create Creator Image data for display if reasonable size
    if (memoryHandle && data && nbytes > 0 && nbytes < 205824) {
        if (unsigned char *imageData = SymbolGroupValue::readMemory(v.context().dataspaces, data, nbytes)) {
            unsigned char *creatorImageData = new unsigned char[sizeof(CreatorImageHeader) + nbytes];
            memcpy(creatorImageData, &header, sizeof(CreatorImageHeader));
            memcpy(creatorImageData + sizeof(CreatorImageHeader), imageData, nbytes);
            delete [] imageData;
            *memoryHandle = new MemoryHandle(creatorImageData, sizeof(CreatorImageHeader) + nbytes);
        }
    }
    return true;
}

// Dump a rectangle in X11 syntax
template <class T>
inline void dumpRect(std::wostream &str, T x, T y, T width, T height)
{
    str << width << 'x' << height;
    if (x >= 0)
        str << '+';
    str << x;
    if (y >= 0)
        str << '+';
    str << y;
}

// Dump Qt's simple geometrical types
static inline bool dumpQSize_F(const SymbolGroupValue &v, std::wostream &str)
{
    str << '(' << v["wd"].value() << ", " << v["ht"].value() << ')';
    return true;
}

static inline bool dumpQPoint_F(const SymbolGroupValue &v, std::wostream &str)
{
    str << '(' << v["xp"].value() << ", " << v["yp"].value() << ')';
    return true;
}

static inline bool dumpQLine_F(const SymbolGroupValue &v, std::wostream &str)
{
    const SymbolGroupValue p1 = v["pt1"];
    const SymbolGroupValue p2 = v["pt2"];
    if (p1 && p2) {
        str << '(' << p1["xp"].value() << ", " << p1["yp"].value() << ") ("
            << p2["xp"].value() << ", " << p2["yp"].value() << ')';
        return true;
    }
    return false;
}

static inline bool dumpQRect(const SymbolGroupValue &v, std::wostream &str)
{
    const int x1 = v["x1"].intValue();
    const int y1 = v["y1"].intValue();
    const int x2 = v["x2"].intValue();
    const int y2 = v["y2"].intValue();
    dumpRect(str, x1, y1, (x2 - x1 + 1), (y2 - y1 + 1));
    return true;
}

static inline bool dumpQRectF(const SymbolGroupValue &v, std::wostream &str)
{
    dumpRect(str, v["xp"].floatValue(), v["yp"].floatValue(), v["w"].floatValue(), v["h"].floatValue());
    return true;
}

/* Return a SymbolGroupValue containing the private class of
 * a type (multiple) derived from QObject and something else
 * (QWidget: QObject,QPaintDevice or QWindow: QObject,QSurface).
 * We get differing behaviour for pointers and values on stack.
 * For 'QWidget *', the base class QObject usually can be accessed
 * by name (past the vtable). When browsing class hierarchies (stack),
 * typically only the uninteresting QPaintDevice is seen. */

SymbolGroupValue qobjectDerivedPrivate(const SymbolGroupValue &v,
                                       const std::string &qwPrivateType,
                                       const QtInfo &qtInfo)
{
    if (const SymbolGroupValue base = v[SymbolGroupValue::stripModuleFromType(qtInfo.qObjectType).c_str()])
        if (const SymbolGroupValue qwPrivate = base["d_ptr"]["d"].pointerTypeCast(qwPrivateType.c_str()))
            return qwPrivate;
    if (!SymbolGroupValue::isPointerType(v.type()))
        return SymbolGroupValue();
    // Class hierarchy: Using brute force, add new symbol based on that
    // QScopedPointer<Private> is basically a 'X *' (first member).
    std::string errorMessage;
    std::ostringstream str;
    str << '(' << qwPrivateType << "*)(" << std::showbase << std::hex << v.address() << ')';
    const std::string name = str.str();
    SymbolGroupNode *qwPrivateNode
        = v.node()->symbolGroup()->addSymbol(v.module(), name, std::string(), &errorMessage);
    return SymbolGroupValue(qwPrivateNode, v.context());
}

static bool dumpQObjectName(const SymbolGroupValue &qoPrivate, std::wostream &str)
{
    // Qt 4: plain member.
    if (QtInfo::get(qoPrivate.context()).version < 5) {
        if (const SymbolGroupValue oName = qoPrivate["objectName"])
            return dumpQString(oName, str);
    }
    // Qt 5: member of allocated extraData.
    if (const SymbolGroupValue extraData = qoPrivate["extraData"])
        if (extraData.pointerValue())
            if (const SymbolGroupValue oName = extraData["objectName"])
                return dumpQString(oName, str);
    return false;
}

// Dump the object name
static inline bool dumpQWidget(const SymbolGroupValue &v, std::wostream &str, void **specialInfoIn = 0)
{
    const QtInfo &qtInfo = QtInfo::get(v.context());
    const SymbolGroupValue qwPrivate =
        qobjectDerivedPrivate(v, qtInfo.qWidgetPrivateType, qtInfo);
    // QWidgetPrivate inherits QObjectPrivate
    if (!qwPrivate || !dumpQObjectName(qwPrivate[unsigned(0)], str))
        return false;
    if (specialInfoIn)
        *specialInfoIn = qwPrivate.node();
    return true;
}

// Dump the object name
static inline bool dumpQObject(const SymbolGroupValue &v, std::wostream &str, void **specialInfoIn = 0)
{
    const std::string &qoPrivateType = QtInfo::get(v.context()).qObjectPrivateType;
    const SymbolGroupValue qoPrivate = v["d_ptr"]["d"].pointerTypeCast(qoPrivateType.c_str());
    if (!qoPrivate || !dumpQObjectName(qoPrivate, str))
        return false;
    if (specialInfoIn)
        *specialInfoIn = qoPrivate.node();
    return true;
}

// Dump the object name
static inline bool dumpQWindow(const SymbolGroupValue &v, std::wostream &str, void **specialInfoIn = 0)
{
    const QtInfo &qtInfo = QtInfo::get(v.context());
    const SymbolGroupValue qwPrivate =
        qobjectDerivedPrivate(v, qtInfo.qWindowPrivateType, qtInfo);
    // QWindowPrivate inherits QObjectPrivate
    if (!qwPrivate || !dumpQObjectName(qwPrivate[unsigned(0)], str))
        return false;
    if (specialInfoIn)
        *specialInfoIn = qwPrivate.node();
    return true;
}

// Dump a std::string.
static bool dumpStd_W_String(const SymbolGroupValue &v, int type, std::wostream &str,
                             MemoryHandle **memoryHandle = 0)
{
    // Find 'bx'. MSVC 2012 has 2 base classes, MSVC 2010 1,
    // and MSVC2008 none
    const SymbolGroupValue bx = SymbolGroupValue::findMember(v, "_Bx");
    const int reserved = bx.parent()["_Myres"].intValue();
    int size = bx.parent()["_Mysize"].intValue();
    if (!bx || reserved < 0 || size < 0)
        return false;
    const bool truncated = unsigned(size) > ExtensionContext::instance().parameters().maxStringLength;
    if (truncated)
        size = ExtensionContext::instance().parameters().maxStringLength;
    // 'Buf' array for small strings, else pointer 'Ptr'.
    const int bufSize = type == KT_StdString ? 16 : 8; // see basic_string.
    const unsigned long memSize = type == KT_StdString ? size : 2 * size;
    const ULONG64 address = reserved >= bufSize ? bx["_Ptr"].pointerValue() : bx["_Buf"].address();
    if (!address)
        return false;
    unsigned char *memory = SymbolGroupValue::readMemory(v.context().dataspaces, address, memSize);
    if (!memory)
        return false;
    str << (type == KT_StdString ?
        quotedWStringFromCharData(memory, memSize, truncated) :
        quotedWStringFromWCharData(memory, memSize, truncated));
    if (memoryHandle)
        *memoryHandle = new MemoryHandle(memory, memSize);
    else
        delete [] memory;
    return true;
}

// QVariant employs a template for storage where anything bigger than the data union
// is pointed to by data.shared.ptr, else it is put into the data struct (pointer size)
// itself (notably Qt types consisting of a d-ptr only).
// The layout can vary between 32 /64 bit for some types: QPoint/QSize (of 2 ints) is bigger
// as a pointer only on 32 bit.

static inline SymbolGroupValue qVariantCast(const SymbolGroupValue &variantData, const char *type)
{
    const ULONG typeSize = SymbolGroupValue::sizeOf(type);
    const std::string ptrType = std::string(type) + " *";
    if (typeSize > variantData.size())
        return variantData["shared"]["ptr"].pointerTypeCast(ptrType.c_str());
    return variantData.typeCast(ptrType.c_str());
}

// Qualify a local container template of Qt Types for QVariant
// as 'QList' of 'QVariant' -> 'localModule!qtnamespace::QList<qtnamespace::QVariant> *'
static inline std::string
    variantContainerType(const std::string &containerType,
                         const std::string &innerType1,
                         const std::string &innerType2 /* = "" */,
                         const QtInfo &qtInfo,
                         const SymbolGroupValue &contextHelper)
{
    std::string rc = QtInfo::prependModuleAndNameSpace(containerType, contextHelper.module(),
                                                       qtInfo.nameSpace);
    rc.push_back('<');
    rc += QtInfo::prependModuleAndNameSpace(innerType1, std::string(), qtInfo.nameSpace);
    if (!innerType2.empty()) {
        rc.push_back(',');
        rc += QtInfo::prependModuleAndNameSpace(innerType2, std::string(), qtInfo.nameSpace);
    }
    rc += "> *";
    return rc;
}

static bool dumpQVariant(const SymbolGroupValue &v, std::wostream &str, void **specialInfoIn = 0)
{
    const QtInfo &qtInfo = QtInfo::get(v.context());
    const SymbolGroupValue dV = v["d"];
    if (!dV)
        return false;
    const SymbolGroupValue typeV = dV["type"];
    const SymbolGroupValue dataV = dV["data"];
    if (!typeV || !dataV)
        return false;
    const int typeId = typeV.intValue();
    if (typeId <= 0) {
        str <<  L"<Invalid>";
        return true;
    }
    switch (typeId) {
    case 1: // Bool
        str << L"(bool) " << dataV["b"].value();
        break;
    case 2: // Int
        str << L"(int) " << dataV["i"].value();
        break;
    case 3: // UInt
        str << L"(unsigned) " << dataV["u"].value();
        break;
    case 4: // LongLong
        str << L"(long long) " << dataV["ll"].value();
        break;
    case 5: // LongLong
        str << L"(unsigned long long) " << dataV["ull"].value();
        break;
    case 6: // Double
        str << L"(double) " << dataV["d"].value();
        break;
    case 7: // Char
        str << L"(char) " << dataV["c"].value();
        break;
    case 8: {
        str << L"(QVariantMap) ";
        const std::string vmType = variantContainerType("QMap", "QString", "QVariant", qtInfo, dataV);
        if (const SymbolGroupValue mv = dataV.typeCast(vmType.c_str())) {
            SymbolGroupNode *mapNode = mv.node();
            std::wstring value;
            if (dumpSimpleType(mapNode, dataV.context(), &value) == SymbolGroupNode::SimpleDumperOk) {
                str << value;
                if (specialInfoIn)
                    *specialInfoIn = mapNode;
            }
        }
    }
        break;
    case 9: { // QVariantList
        str << L"(QVariantList) ";
        const std::string vLType = variantContainerType("QList", "QVariant", std::string(), qtInfo, dataV);
        if (const SymbolGroupValue vl = dataV.typeCast(vLType.c_str())) {
            SymbolGroupNode *vListNode = vl.node();
            std::wstring value;
            if (dumpSimpleType(vListNode, dataV.context(), &value) == SymbolGroupNode::SimpleDumperOk) {
                str << value;
                if (specialInfoIn)
                    *specialInfoIn = vListNode;
            }
        }
    }
        break;
    case 10: // String
        str << L"(QString) ";
        if (const SymbolGroupValue sv = dataV.typeCast(qtInfo.prependQtCoreModule("QString *").c_str()))
            dumpQString(sv, str);
        break;
    case 11: //StringList: Dump container size
        str << L"(QStringList) ";
        if (const SymbolGroupValue sl = dataV.typeCast(qtInfo.prependQtCoreModule("QStringList *").c_str())) {
            SymbolGroupNode *listNode = sl.node();
            std::wstring value;
            if (dumpSimpleType(listNode, dataV.context(), &value) == SymbolGroupNode::SimpleDumperOk) {
                str << value;
                if (specialInfoIn)
                    *specialInfoIn = listNode;
            }
        }
        break;
    case 12: //ByteArray
        str << L"(QByteArray) ";
        if (const SymbolGroupValue sv = dataV.typeCast(qtInfo.prependQtCoreModule("QByteArray *").c_str()))
            dumpQByteArray(sv, str);
        break;
    case 13: // BitArray
        str << L"(QBitArray)";
        break;
    case 14: // Date: Do not qualify - fails non-deterministically with QtCored4!QDate
        str << L"(QDate) ";
        if (const SymbolGroupValue sv = dataV.typeCast("QDate *"))
            dumpQDate(sv, str);
        break;
    case 15: // Time: Do not qualify - fails non-deterministically with QtCored4!QTime
        str << L"(QTime) ";
        if (const SymbolGroupValue sv = dataV.typeCast("QTime *"))
            dumpQTime(sv, str);
        break;
    case 16: // DateTime
        str << L"(QDateTime)";
        break;
    case 17: // Url
        str << L"(QUrl)";
        break;
    case 18: // Locale
        str << L"(QLocale)";
        break;
    case 19: // Rect:
        str << L"(QRect) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast(qtInfo.prependQtCoreModule("QRect *").c_str()))
            dumpQRect(sv, str);
        break;
    case 20: // RectF
        str << L"(QRectF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast(qtInfo.prependQtCoreModule("QRectF *").c_str()))
            dumpQRectF(sv, str);
        break;
    case 21: // Size
        // Anything bigger than the data union is a pointer, else the data union is used
        str << L"(QSize) ";
        if (const SymbolGroupValue sv = qVariantCast(dataV, qtInfo.prependQtCoreModule("QSize").c_str()))
            dumpQSize_F(sv, str);
        break;
    case 22: // SizeF
        str << L"(QSizeF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast(qtInfo.prependQtCoreModule("QSizeF *").c_str()))
            dumpQSize_F(sv, str);
        break;
    case 23: // Line
        str << L"(QLine) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast(qtInfo.prependQtCoreModule("QLine *").c_str()))
            dumpQLine_F(sv, str);
        break;
    case 24: // LineF
        str << L"(QLineF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast(qtInfo.prependQtCoreModule("QLineF *").c_str()))
            dumpQLine_F(sv, str);
        break;
    case 25: // Point
        str << L"(QPoint) ";
        if (const SymbolGroupValue sv = qVariantCast(dataV, qtInfo.prependQtCoreModule("QPoint").c_str()))
            dumpQPoint_F(sv, str);
        break;
    case 26: // PointF
        str << L"(QPointF) ";
        if (const SymbolGroupValue sv = dataV["shared"]["ptr"].pointerTypeCast(qtInfo.prependQtCoreModule("QPointF *").c_str()))
            dumpQPoint_F(sv, str);
        break;
    default:
        str << L"Type " << typeId;
        break;
    }
    return true;
}

// Dump a qsharedpointer (just list reference counts)
static inline bool dumpQSharedPointer(const SymbolGroupValue &v, std::wostream &str, void **specialInfoIn = 0)
{
    const SymbolGroupValue externalRefCountV = v[unsigned(0)];
    if (!externalRefCountV)
        return false;
    const SymbolGroupValue dV = externalRefCountV["d"];
    if (!dV)
        return false;
    // Get value element from base and store in special info.
    const SymbolGroupValue valueV = externalRefCountV[unsigned(0)]["value"];
    if (!valueV)
        return false;
    // Format references.
    const int strongRef = dV["strongref"]["_q_value"].intValue();
    const int weakRef = dV["weakref"]["_q_value"].intValue();
    if (strongRef < 0 || weakRef < 0)
        return false;
    str << L"References: " << strongRef << '/' << weakRef;
    *specialInfoIn = valueV.node();
    return true;
}

// Dump builtin simple types using SymbolGroupValue expressions.
unsigned dumpSimpleType(SymbolGroupNode  *n, const SymbolGroupValueContext &ctx,
                        std::wstring *s, int *knownTypeIn /* = 0 */,
                        int *containerSizeIn /* = 0 */,
                        void **specialInfoIn /* = 0 */,
                        MemoryHandle **memoryHandleIn /* = 0 */)
{
    QTC_TRACE_IN
    if (containerSizeIn)
        *containerSizeIn = -1;
    if (specialInfoIn)
        *specialInfoIn  = 0;
    // Check for class types and strip pointer types (references appear as pointers as well)
    s->clear();
    const KnownType kt = knownType(n->type(), KnownTypeHasClassPrefix|KnownTypeAutoStripPointer);
    if (knownTypeIn)
        *knownTypeIn = kt;

    if (kt == KT_Unknown || !(kt & KT_HasSimpleDumper)) {
        if (SymbolGroupValue::verbose > 1)
            DebugPrint() << "dumpSimpleType N/A " << n->name() << '/' << n->type();
        QTC_TRACE_OUT
        return SymbolGroupNode::SimpleDumperNotApplicable;
    }

    std::wostringstream str;

    // Prefix by pointer value
    const SymbolGroupValue v(n, ctx);
    if (!v) // Value as such has memory read error?
        return SymbolGroupNode::SimpleDumperFailed;
    if (SymbolGroupValue::isPointerType(v.type()))
        if (const ULONG64 pointerValue = v.pointerValue())
            str << std::showbase << std::hex << pointerValue << std::dec << std::noshowbase << ' ';

    // Simple dump of size for containers
    if (kt & KT_ContainerType) {
        const int size = containerSize(kt, v);
        if (SymbolGroupValue::verbose > 1)
            DebugPrint() << "dumpSimpleType Container " << n->name() << '/' << n->type() << " size=" << size;
        if (containerSizeIn)
            *containerSizeIn = size;
        if (size >= 0) {
            str << L'<' << size << L" items>";
            *s = str.str();
            return SymbolGroupNode::SimpleDumperOk;
        }
        return SymbolGroupNode::SimpleDumperFailed;
    }
    unsigned rc = SymbolGroupNode::SimpleDumperNotApplicable;
    switch (kt) {
    case KT_QChar:
        rc = dumpQChar(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QByteArray:
        rc = dumpQByteArray(v, str, memoryHandleIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QFileInfo:
        rc = dumpQFileInfo(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QFile:
        rc = dumpQFile(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QDir:
        rc = dumpQDir(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QRegExp:
        rc = dumpQRegExp(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QUrl:
        rc = dumpQUrl(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QHostAddress:
        rc = dumpQHostAddress(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QProcess:
        rc = dumpQProcess(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QScriptValue:
        rc = dumpQScriptValue(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QString:
        rc = dumpQString(v, str, memoryHandleIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QStringRef:
        rc = dumpQStringRef(v, str, memoryHandleIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QColor:
        rc = dumpQColor(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QFlags:
        rc = dumpQFlags(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QDate:
        rc = dumpQDate(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QTime:
        rc = dumpQTime(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QDateTime:
        rc = dumpQDateTime(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QPoint:
    case KT_QPointF:
        rc = dumpQPoint_F(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QSize:
    case KT_QSizeF:
        rc = dumpQSize_F(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QLine:
    case KT_QLineF:
        rc = dumpQLine_F(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QImage:
        rc = dumpQImage(v, str, memoryHandleIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QRect:
        rc = dumpQRect(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QRectF:
        rc = dumpQRectF(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QVariant:
        rc = dumpQVariant(v, str, specialInfoIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QAtomicInt:
        rc = dumpQAtomicInt(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QBasicAtomicInt:
        rc = dumpQBasicAtomicInt(v, str) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QObject:
        rc = dumpQObject(v, str, specialInfoIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QWidget:
        rc = dumpQWidget(v, str, specialInfoIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QWindow:
        rc = dumpQWindow(v, str, specialInfoIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_QSharedPointer:
        rc = dumpQSharedPointer(v, str, specialInfoIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    case KT_StdString:
    case KT_StdWString:
        rc = dumpStd_W_String(v, kt, str, memoryHandleIn) ? SymbolGroupNode::SimpleDumperOk : SymbolGroupNode::SimpleDumperFailed;
        break;
    default:
        break;
    }
    if (rc == SymbolGroupNode::SimpleDumperOk)
        *s = str.str();
    QTC_TRACE_OUT

    if (SymbolGroupValue::verbose > 1) {
        DebugPrint dp;
        dp << "dumpSimpleType " << n->name() << '/' << n->type() << " knowntype= " << kt << " [";
        formatKnownTypeFlags(dp, kt);
        dp << "] returns " << rc;
    }
    return rc;
}

static inline void formatEditValue(int displayFormat, const MemoryHandle *mh, std::ostream &str)
{
    str << "editformat=\"" << displayFormat << "\",editvalue=\""
        << mh->toHex() << "\",";
}

bool dumpEditValue(const SymbolGroupNode *n, const SymbolGroupValueContext &,
                   int desiredFormat, std::ostream &str)
{
    // Keep in sync watchhandler.cpp/showEditValue(), dumper.py.
    enum DebuggerEditFormats {
        DisplayImageData                       = 1,
        DisplayUtf16String                     = 2,
        DisplayImageFile                       = 3,
        DisplayProcess                         = 4,
        DisplayLatin1String                    = 5,
        DisplayUtf8String                      = 6
    };

    enum Formats {
        NormalFormat = 0,
        StringSeparateWindow = 1 // corresponds to menu index.
    };

    if (desiredFormat <= 0)
        return true;

    if (SymbolGroupValue::verbose)
        DebugPrint() << __FUNCTION__ << ' ' << n->name() << '/' << desiredFormat;

    switch (n->dumperType()) {
    case KT_QString:
    case KT_StdWString:
        if (desiredFormat == StringSeparateWindow)
            if (const MemoryHandle *mh = n->memory())
                formatEditValue(DisplayUtf16String, mh, str);
        break;
    case KT_QByteArray:
    case KT_StdString:
        if (desiredFormat == StringSeparateWindow)
            if (const MemoryHandle *mh = n->memory())
                formatEditValue(DisplayLatin1String, mh, str);
        break;
    case KT_QImage:
        if (desiredFormat == 1) // Image.
            if (const MemoryHandle *mh = n->memory())
                formatEditValue(DisplayImageData, mh, str);
        break;
    }
    return true;
}

// Dump of QByteArray: Display as an array of unsigned chars.
static inline std::vector<AbstractSymbolGroupNode *>
    complexDumpQByteArray(SymbolGroupNode *n, const SymbolGroupValueContext &ctx)
{
    std::vector<AbstractSymbolGroupNode *> rc;

    const SymbolGroupValue baV(n, ctx);
    const SymbolGroupValue dV = baV["d"];
    if (!dV)
        return rc;
    // Determine memory area.

    unsigned size = 0;
    ULONG64 address = 0;
    if (QtInfo::get(ctx).version > 4) {
        const QtStringAddressData data = readQtStringAddressData(dV, 5);
        size = data.size;
        address = data.address;
    } else {
        size = dV["size"].intValue();
        address = dV["data"].pointerValue();
    }

    if (size <= 0 || !address)
        return rc;

    if (size > 200)
        size = 200;
    rc.reserve(size);
    const std::string charType = "unsigned char";
    std::string errorMessage;
    SymbolGroup *sg = n->symbolGroup();
    for (int i = 0; i < (int)size; ++i, ++address) {
        SymbolGroupNode *en = sg->addSymbol(std::string(), SymbolGroupValue::pointedToSymbolName(address, charType),
                                            std::string(), &errorMessage);
        if (!en) {
            rc.clear();
            return rc;
        }
        rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, en));
    }
    return rc;
}

/* AssignmentStringData: Helper struct used for assigning values
 * to string classes. Contains an (allocated) data array with size for use
 * with IDebugDataSpaced::FillVirtual() + string length information and
 * provides a conversion function decodeString() to create the array
 * depending on the argument format (blow up ASCII to UTF16 or vice versa). */

struct AssignmentStringData
{
    explicit AssignmentStringData(size_t dataLengthIn, size_t stringLengthIn);
    static AssignmentStringData decodeString(const char *begin, const char *end,
                                   int valueEncoding, bool toUtf16);

    static inline AssignmentStringData decodeString(const std::string &value,
                                        int valueEncoding, bool toUtf16)
        { return decodeString(value.c_str(), value.c_str() + value.size(),
                              valueEncoding, toUtf16); }

    unsigned char *data;
    size_t dataLength;
    size_t stringLength;
};

AssignmentStringData::AssignmentStringData(size_t dataLengthIn, size_t stringLengthIn) :
    data(new unsigned char[dataLengthIn]), dataLength(dataLengthIn),
    stringLength(stringLengthIn)
{
    if (dataLength)
        memset(data, 0, dataLength);
}

AssignmentStringData AssignmentStringData::decodeString(const char *begin, const char *end,
                                    int valueEncoding, bool toUtf16)
{
    if (toUtf16) { // Target is UTF16 consisting of unsigned short characters.
        switch (valueEncoding) {
        // Hex encoded ASCII/2 digits per char: Decode to plain characters and
        // recurse to expand them.
        case AssignHexEncoded: {
            const AssignmentStringData decoded = decodeString(begin, end, AssignHexEncoded, false);
            const char *source = reinterpret_cast<const char*>(decoded.data);
            const AssignmentStringData utf16 = decodeString(source, source + decoded.stringLength,
                                                  AssignPlainValue, true);
            delete [] decoded.data;
            return utf16;
        }
        // Hex encoded UTF16: 4 hex digits per character: Decode sequence.
        case AssignHexEncodedUtf16: {
            const size_t stringLength = (end - begin) / 4;
            AssignmentStringData result(sizeof(unsigned short) *(stringLength + 1), stringLength);
            decodeHex(begin, end, result.data);
            return result;
        }
        default:
            break;
        }
        // Convert plain ASCII data to UTF16 by expanding.
        const size_t stringLength = end - begin;
        AssignmentStringData result(sizeof(unsigned short) *(stringLength + 1), stringLength);
        const unsigned char *source = reinterpret_cast<const unsigned char *>(begin);
        unsigned short *target = reinterpret_cast<unsigned short *>(result.data);
        std::copy(source, source + stringLength, target);

        return result;
    } // toUtf16
    switch (valueEncoding) {
    case AssignHexEncoded: { // '0A5A'..2 digits per character
        const size_t stringLength = (end - begin) / 2;
        AssignmentStringData result(stringLength + 1, stringLength);
        decodeHex(begin, end, result.data);
        return result;
    }
    // Condense UTF16 characters to ASCII: Decode and use only every 2nd character
    // (little endian, first byte)
    case AssignHexEncodedUtf16: {
        const AssignmentStringData decoded = decodeString(begin, end, AssignHexEncoded, false);
        const size_t stringLength = decoded.stringLength / 2;
        const AssignmentStringData result(stringLength + 1, stringLength);
        const unsigned char *sourceEnd = decoded.data + decoded.stringLength;
        unsigned char *target = result.data;
        for (const unsigned char *source = decoded.data; source < sourceEnd; source += 2)
            *target++ = *source;
        delete [] decoded.data;
        return result;
    }
        break;
    default:
        break;
    }
    // Plain 0-terminated copy
    const size_t stringLength = end - begin;
    AssignmentStringData result(stringLength + 1, stringLength);
    memcpy(result.data, begin, stringLength);
    return result;
}

// Assignment helpers
static inline std::string msgAssignStringFailed(const std::string &value, int errorCode)
{
    std::ostringstream estr;
    estr << "Unable to assign a string of " << value.size() << " bytes: Error " << errorCode;
    return estr.str();
}

/* QString assign helper: If QString instance has sufficiently allocated,
 * memory, write the data. Else invoke 'QString::resize' and
 * recurse (since 'd' might become invalid). This works for QString with UTF16
 * data and for QByteArray with ASCII data due to the similar member
 * names and both using a terminating '\0' w_char/byte. */
static int assignQStringI(SymbolGroupNode  *n, const char *className,
                          const AssignmentStringData &data,
                          const SymbolGroupValueContext &ctx,
                          bool doAlloc = true)
{
    const SymbolGroupValue v(n, ctx);
    SymbolGroupValue d = v["d"];
    if (!d)
        return 1;
    const QtInfo &qtInfo = QtInfo::get(ctx);
    // Check the size, re-allocate if required.
    const QtStringAddressData addressData = readQtStringAddressData(d, qtInfo.version);
    if (!addressData.address)
        return 9;
    const bool needRealloc = addressData.allocated < data.stringLength;
    if (needRealloc) {
        if (!doAlloc) // Calling re-alloc failed somehow.
            return 3;
        std::ostringstream callStr;
        const std::string funcName
            = qtInfo.prependQtCoreModule(std::string(className) + "::resize");
        callStr << funcName << '(' << std::hex << std::showbase
                << v.address() << ',' << data.stringLength << ')';
        std::wstring wOutput;
        std::string errorMessage;
        return ExtensionContext::instance().call(callStr.str(), &wOutput, &errorMessage) ?
            assignQStringI(n, className, data, ctx, false) : 5;
    }
    // Write data.
    if (!SymbolGroupValue::writeMemory(v.context().dataspaces,
                                       addressData.address, data.data,
                                       ULONG(data.dataLength)))
        return 11;
    // Correct size unless we re-allocated
    if (!needRealloc) {
        const SymbolGroupValue size = d["size"];
        if (!size || !size.node()->assign(toString(data.stringLength)))
            return 17;
    }
    return 0;
}

// QString assignment
static inline bool assignQString(SymbolGroupNode  *n,
                                 int valueEncoding, const std::string &value,
                                 const SymbolGroupValueContext &ctx,
                                 std::string *errorMessage)
{
    const AssignmentStringData utf16 = AssignmentStringData::decodeString(value, valueEncoding, true);
    const int errorCode = assignQStringI(n, "QString", utf16, ctx);
    delete [] utf16.data;
    if (errorCode) {
        *errorMessage = msgAssignStringFailed(value, errorCode);
        return false;
    }
    return true;
}

// QByteArray assignment
static inline bool assignQByteArray(SymbolGroupNode  *n,
                                    int valueEncoding, const std::string &value,
                                    const SymbolGroupValueContext &ctx,
                                    std::string *errorMessage)
{
    const AssignmentStringData data = AssignmentStringData::decodeString(value, valueEncoding, false);
    const int errorCode = assignQStringI(n, "QByteArray", data, ctx);
    delete [] data.data;
    if (errorCode) {
        *errorMessage = msgAssignStringFailed(value, errorCode);
        return false;
    }
    return true;
}

// Helper to assign character data to std::string or std::wstring.
static inline int assignStdStringI(SymbolGroupNode  *n, int type,
                                   const AssignmentStringData &data,
                                   const SymbolGroupValueContext &ctx)
{
    /* We do not reallocate and just write to the allocated buffer
     * (internal small buffer or _Ptr depending on reserved) provided sufficient
     * memory is there since it is apparently not possible to call the template
     * function std::string::resize().
     * See the dumpStd_W_String() how to figure out if the internal buffer
     * or an allocated array is used. */

    const SymbolGroupValue v(n, ctx);
    SymbolGroupValue bx = v[unsigned(0)]["_Bx"];
    SymbolGroupValue size;
    int reserved = 0;
    if (bx) { // MSVC2010
        size = v[unsigned(0)]["_Mysize"];
        reserved = v[unsigned(0)]["_Myres"].intValue();
    } else { // MSVC2008
        bx = v["_Bx"];
        size = v["_Mysize"];
        reserved = v["_Myres"].intValue();
    }
    if (reserved < 0 || !size || !bx)
        return 42;
    if (reserved <= (int)data.stringLength)
        return 1; // Insufficient memory.
    // Copy data: 'Buf' array for small strings, else pointer 'Ptr'.
    const int bufSize = type == KT_StdString ? 16 : 8; // see basic_string.
    const ULONG64 address = (bufSize <= reserved) ?
                            bx["_Ptr"].pointerValue() : bx["_Buf"].address();
    if (!address)
        return 3;
    if (!SymbolGroupValue::writeMemory(v.context().dataspaces,
                                       address, data.data, ULONG(data.dataLength)))
        return 7;
    // Correct size
    if (!size.node()->assign(toString(data.stringLength)))
        return 13;
    return 0;
}

// assignment of std::string assign via ASCII, std::wstring via UTF16
static inline bool assignStdString(SymbolGroupNode  *n,
                                   int type, int valueEncoding, const std::string &value,
                                   const SymbolGroupValueContext &ctx,
                                   std::string *errorMessage)
{
    const bool toUtf16 = type == KT_StdWString;
    const AssignmentStringData data = AssignmentStringData::decodeString(value, valueEncoding,
                                                                         toUtf16);
    const int errorCode = assignStdStringI(n, type, data, ctx);
    delete [] data.data;
    if (errorCode) {
        *errorMessage = msgAssignStringFailed(value, errorCode);
        return false;
    }
    return true;
}

bool assignType(SymbolGroupNode  *n, int valueEncoding, const std::string &value,
                const SymbolGroupValueContext &ctx, std::string *errorMessage)
{
    switch (n->dumperType()) {
    case KT_QString:
        return assignQString(n, valueEncoding, value, ctx, errorMessage);
    case KT_QByteArray:
        return assignQByteArray(n, valueEncoding, value, ctx, errorMessage);
    case KT_StdString:
    case KT_StdWString:
        return assignStdString(n, n->dumperType(), valueEncoding, value, ctx, errorMessage);
    default:
        break;
    }
    return false;
}

std::vector<AbstractSymbolGroupNode *>
    dumpComplexType(SymbolGroupNode *n, int type, void *specialInfo,
                    const SymbolGroupValueContext &ctx)
{
    std::vector<AbstractSymbolGroupNode *> rc;
    if (!(type & KT_HasComplexDumper))
        return rc;
    switch (type) {
    case KT_QByteArray:
        rc = complexDumpQByteArray(n, ctx);
        break;
    case KT_QWidget: // Special info by simple dumper is the QWidgetPrivate node
    case KT_QWindow: // Special info by simple dumper is the QWindowPrivate node
    case KT_QObject: // Special info by simple dumper is the QObjectPrivate node
        if (specialInfo) {
            SymbolGroupNode *qObjectPrivateNode = reinterpret_cast<SymbolGroupNode *>(specialInfo);
            rc.push_back(new ReferenceSymbolGroupNode("d", "d", qObjectPrivateNode));
        }
        break;
    case KT_QVariant: // Special info by simple dumper is the container (stringlist, map,etc)
        if (specialInfo) {
            SymbolGroupNode *containerNode = reinterpret_cast<SymbolGroupNode *>(specialInfo);
            rc.push_back(new ReferenceSymbolGroupNode("children", "children", containerNode));
        }
    case KT_QSharedPointer: // Special info by simple dumper is the value
        if (specialInfo) {
            SymbolGroupNode *valueNode = reinterpret_cast<SymbolGroupNode *>(specialInfo);
            rc.push_back(new ReferenceSymbolGroupNode("value", "value", valueNode));
        }
        break;
    default:
        break;
    }
    if (SymbolGroupValue::verbose) {
        DebugPrint dp;
        dp << "<dumpComplexType" << rc.size() << ' ';
        for (VectorIndexType i = 0; i < rc.size() ; ++i)
            dp << i << ' ' << rc.at(i)->name();
    }
    return rc;
}
