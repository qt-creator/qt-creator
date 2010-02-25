/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "symbolgroupcontext.h"
#include "coreengine.h"

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QRegExp>
#include <QtCore/QString>
#include <QtCore/QDebug>

enum { debug = 0 };
enum { debugInternalDumpers = 1 };

// name separator for shadowed variables
static const char iNameShadowDelimiter = '#';

static inline QString msgSymbolNotFound(const QString &s)
{
    return QString::fromLatin1("The symbol '%1' could not be found.").arg(s);
}

static inline QString msgOutOfScope()
{
    return QCoreApplication::translate("SymbolGroup", "Out of scope");
}

static inline bool isTopLevelSymbol(const DEBUG_SYMBOL_PARAMETERS &p)
{
    return p.ParentSymbol == DEBUG_ANY_ID;
}

static inline void debugSymbolFlags(unsigned long f, QTextStream &str)
{
    if (f & DEBUG_SYMBOL_EXPANDED)
        str << "DEBUG_SYMBOL_EXPANDED";
    if (f & DEBUG_SYMBOL_READ_ONLY)
        str << "|DEBUG_SYMBOL_READ_ONLY";
    if (f & DEBUG_SYMBOL_IS_ARRAY)
        str << "|DEBUG_SYMBOL_IS_ARRAY";
    if (f & DEBUG_SYMBOL_IS_FLOAT)
        str << "|DEBUG_SYMBOL_IS_FLOAT";
    if (f & DEBUG_SYMBOL_IS_ARGUMENT)
        str << "|DEBUG_SYMBOL_IS_ARGUMENT";
    if (f & DEBUG_SYMBOL_IS_LOCAL)
        str << "|DEBUG_SYMBOL_IS_LOCAL";
}

QTextStream &operator<<(QTextStream &str, const DEBUG_SYMBOL_PARAMETERS &p)
{
    str << " Type=" << p.TypeId << " parent=";
    if (isTopLevelSymbol(p)) {
        str << "<ROOT>";
    } else {
        str << p.ParentSymbol;
    }
    str << " Subs=" << p.SubElements << " flags=" << p.Flags << '/';
    debugSymbolFlags(p.Flags, str);
    return str;
}

static inline ULONG64 symbolOffset(CIDebugSymbolGroup *sg, unsigned long index)
{
    ULONG64 rc = 0;
    if (FAILED(sg->GetSymbolOffset(index, &rc)))
        rc = 0;
    return rc;
}

// A helper function to extract a string value from a member function of
// IDebugSymbolGroup2 taking the symbol index and a character buffer.
// Pass in the the member function as '&IDebugSymbolGroup2::GetSymbolNameWide'

typedef HRESULT  (__stdcall IDebugSymbolGroup2::*WideStringRetrievalFunction)(ULONG, PWSTR, ULONG, PULONG);

static inline QString getSymbolString(IDebugSymbolGroup2 *sg,
                                      WideStringRetrievalFunction wsf,
                                      unsigned long index)
{
    // Template type names can get quite long....
    enum { BufSize = 1024 };
    static WCHAR nameBuffer[BufSize + 1];
    // Name
    ULONG nameLength;
    const HRESULT hr = (sg->*wsf)(index, nameBuffer, BufSize, &nameLength);
    if (SUCCEEDED(hr)) {
        nameBuffer[nameLength] = 0;
        return QString::fromUtf16(reinterpret_cast<const ushort *>(nameBuffer));
    }
    return QString();
}

namespace CdbCore {

static inline SymbolGroupContext::SymbolState getSymbolState(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.SubElements == 0u)
        return SymbolGroupContext::LeafSymbol;
    return (p.Flags & DEBUG_SYMBOL_EXPANDED) ?
               SymbolGroupContext::ExpandedSymbol :
               SymbolGroupContext::CollapsedSymbol;
}

SymbolGroupContext::SymbolGroupContext(const QString &prefix,
                                       CIDebugSymbolGroup *symbolGroup,
                                       CIDebugDataSpaces *dataSpaces,
                                       const QStringList &uninitializedVariables) :
    m_prefix(prefix),
    m_nameDelimiter(QLatin1Char('.')),
    m_uninitializedVariables(uninitializedVariables.toSet()),
    m_symbolGroup(symbolGroup),
    m_dataSpaces(dataSpaces),
    m_unnamedSymbolNumber(1),
    m_shadowedNameFormat(QLatin1String("%1#%2"))
{
}

SymbolGroupContext::~SymbolGroupContext()
{
    m_symbolGroup->Release();
}

SymbolGroupContext *SymbolGroupContext::create(const QString &prefix,
                                                     CIDebugSymbolGroup *symbolGroup,
                                                     CIDebugDataSpaces *dataSpaces,
                                                     const QStringList &uninitializedVariables,
                                                     QString *errorMessage)
{
    SymbolGroupContext *rc = new SymbolGroupContext(prefix, symbolGroup, dataSpaces, uninitializedVariables);
    if (!rc->init(errorMessage)) {
        delete rc;
        return 0;
    }
    return rc;
}

bool SymbolGroupContext::init(QString *errorMessage)
{
    // retrieve the root symbols
    ULONG count;
    HRESULT hr = m_symbolGroup->GetNumberSymbols(&count);
    if (FAILED(hr)) {
        *errorMessage = CdbCore::msgComFailed("GetNumberSymbols", hr);
        return false;
    }

    if (count) {
        m_symbolParameters.reserve(3u * count);
        m_symbolParameters.resize(count);

        hr = m_symbolGroup->GetSymbolParameters(0, count, symbolParameters());
        if (FAILED(hr)) {
            *errorMessage = QString::fromLatin1("In %1: %2 (%3 symbols)").arg(QLatin1String(Q_FUNC_INFO),
                                                                              CdbCore::msgComFailed("GetSymbolParameters", hr)).arg(count);
            return false;
        }
        populateINameIndexMap(m_prefix, DEBUG_ANY_ID, count);
    }
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n'<< debugToString();
    return true;
}

QString SymbolGroupContext::shadowedNameFormat() const
{
    return m_shadowedNameFormat;
}

void SymbolGroupContext::setShadowedNameFormat(const QString &f)
{
    m_shadowedNameFormat = f;
}

/* Make the entries for iname->index mapping. We might encounter
 * already expanded subitems when doing it for top-level ('this'-pointers),
 * recurse in that case, (skip over expanded children).
 * Loop backwards to detect shadowed variables in the order the
/* debugger expects them:
\code
int x;             // Occurrence (1), should be reported as "x <shadowed 1>"
if (true) {
   int x = 5; (2)  // Occurrence (2), should be reported as "x"
}
\endcode
 * The order in the symbol group is (1),(2). Give them an iname of
 * <root>#<shadowed-nr>, which will be split apart for display. */

void SymbolGroupContext::populateINameIndexMap(const QString &prefix, unsigned long parentId,
                                                  unsigned long end)
{
    const QString symbolPrefix = prefix + m_nameDelimiter;
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n'<< symbolPrefix << parentId << end;
    for (unsigned long i = end - 1; ; i--) {
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(i);
        if (parentId == p.ParentSymbol) {
            // "__formal" occurs when someone writes "void foo(int /* x */)..."
            static const QString unnamedFormalParameter = QLatin1String("__formal");
            QString symbolName = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolNameWide, i);
            if (symbolName == unnamedFormalParameter) {
                symbolName = QLatin1String("<unnamed");
                symbolName += QString::number(m_unnamedSymbolNumber++);
                symbolName += QLatin1Char('>');
            }
            // Find a unique name in case the variable is shadowed by
            // an existing one
            const QString namePrefix = symbolPrefix + symbolName;
            QString name = namePrefix;
            for (int n = 1; m_inameIndexMap.contains(name); n++) {
                name.truncate(namePrefix.size());
                name += QLatin1Char(iNameShadowDelimiter);
                name += QString::number(n);
            }
            m_inameIndexMap.insert(name, i);
            if (getSymbolState(p) == ExpandedSymbol)
                populateINameIndexMap(name, i, i + 1 + p.SubElements);
        }
        if (i == 0 || i == parentId)
            break;
    }
}

QString SymbolGroupContext::toString()
{
    QString rc;
    QTextStream str(&rc);
    const unsigned long count = m_symbolParameters.size();
    QString iname;
    QString name;
    ULONG64 addr;
    ULONG typeId;
    QString typeName;
    QString value;


    for (unsigned long i = 0; i < count; i++) {
        const unsigned rc = dumpValue(i, &iname, &name, &addr,
                                      &typeId, &typeName, &value);
        str << iname << ' ' << name << ' ' << typeName << " (" << typeId
                << ") '" << value;
        str.setIntegerBase(16);
        str << "' 0x" << addr << " flags: 0x"  <<rc << '\n';
        str.setIntegerBase(10);
    } // for

    return rc;
}

QString SymbolGroupContext::debugToString(bool verbose) const
{
    QString rc;
    QTextStream str(&rc);
    const int count = m_symbolParameters.size();
    for (int i = 0; i < count; i++) {
        str << i << ' ';
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(i);
        if (!isTopLevelSymbol(p))
            str << "    ";
        str << getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolNameWide, i);
        if (p.Flags & DEBUG_SYMBOL_IS_LOCAL)
            str << " '" << getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolTypeNameWide, i) << '\'';
        str << " Address: " << symbolOffset(m_symbolGroup, i);
        if (verbose)
            str << " '" << getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolValueTextWide, i) << '\'';
        str << p << '\n';
    }
    if (verbose) {
        str << "NameIndexMap\n";
        NameIndexMap::const_iterator ncend = m_inameIndexMap.constEnd();
        for (NameIndexMap::const_iterator it = m_inameIndexMap.constBegin() ; it != ncend; ++it)
            str << it.key() << ' ' << it.value() << '\n';
    }
    return rc;
}

SymbolGroupContext::SymbolState SymbolGroupContext::symbolState(unsigned long index) const
{
    return getSymbolState(m_symbolParameters.at(index));
}

SymbolGroupContext::SymbolState SymbolGroupContext::symbolState(const QString &prefix) const
{
    if (prefix == m_prefix) // root
        return ExpandedSymbol;
    unsigned long index;
    if (!lookupPrefix(prefix, &index)) {
        qWarning("WARNING %s: %s\n", Q_FUNC_INFO, qPrintable(msgSymbolNotFound(prefix)));
        return LeafSymbol;
    }
    return symbolState(index);
}

// Find index of a prefix
bool SymbolGroupContext::lookupPrefix(const QString &prefix, unsigned long *index) const
{
    *index = 0;
    const NameIndexMap::const_iterator it = m_inameIndexMap.constFind(prefix);
    if (it == m_inameIndexMap.constEnd())
        return false;
    *index = it.value();
    return true;
}

/* Retrieve children and get the position. */
bool SymbolGroupContext::getChildSymbolsPosition(const QString &prefix,
                                                    unsigned long *start,
                                                    unsigned long *parentId,
                                                    QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n'<< prefix;

    *start = *parentId = 0;
    // Root item?
    if (prefix == m_prefix) {
        *start = 0;
        *parentId = DEBUG_ANY_ID;
        if (debug)
            qDebug() << '<' << prefix << "at" << *start;
        return true;
    }
    // Get parent index, make sure it is expanded
    NameIndexMap::const_iterator nit = m_inameIndexMap.constFind(prefix);
    if (nit == m_inameIndexMap.constEnd()) {
        *errorMessage = QString::fromLatin1("'%1' not found.").arg(prefix);
        return false;
    }
    *parentId = nit.value();
    *start = nit.value() + 1;
    if (!expandSymbol(prefix, *parentId, errorMessage))
        return false;
    if (debug)
        qDebug() << '<' << prefix << "at" << *start;
    return true;
}

static inline QString msgExpandFailed(const QString &prefix, unsigned long index, const QString &why)
{
    return QString::fromLatin1("Unable to expand '%1' %2: %3").arg(prefix).arg(index).arg(why);
}

// Expand a symbol using the symbol group interface.
bool SymbolGroupContext::expandSymbol(const QString &prefix, unsigned long index, QString *errorMessage)
{
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << '\n' << prefix << index;

    switch (symbolState(index)) {
    case LeafSymbol:
        *errorMessage = QString::fromLatin1("Attempt to expand leaf node '%1' %2!").arg(prefix).arg(index);
        return false;
    case ExpandedSymbol:
        return true;
    case CollapsedSymbol:
        break;
    }

    HRESULT hr = m_symbolGroup->ExpandSymbol(index, TRUE);
    if (FAILED(hr)) {
        *errorMessage = msgExpandFailed(prefix, index, CdbCore::msgComFailed("ExpandSymbol", hr));
        return false;
    }
    // Hopefully, this will never fail, else data structure will be foobar.
    const ULONG oldSize = m_symbolParameters.size();
    ULONG newSize;
    hr = m_symbolGroup->GetNumberSymbols(&newSize);
    if (FAILED(hr)) {
        *errorMessage = msgExpandFailed(prefix, index, CdbCore::msgComFailed("GetNumberSymbols", hr));
        return false;
    }

    // Retrieve the new parameter structs which will be inserted
    // after the parents, offsetting consecutive indexes.
    m_symbolParameters.resize(newSize);

    hr = m_symbolGroup->GetSymbolParameters(0, newSize, symbolParameters());
    if (FAILED(hr)) {
        *errorMessage = msgExpandFailed(prefix, index, CdbCore::msgComFailed("GetSymbolParameters", hr));
        return false;
    }
    // The new symbols are inserted after the parent symbol.
    // We need to correct the following values in the name->index map
    const unsigned long newSymbolCount = newSize - oldSize;
    const NameIndexMap::iterator nend = m_inameIndexMap.end();
    for (NameIndexMap::iterator it = m_inameIndexMap.begin(); it != nend; ++it)
        if (it.value() > index)
            it.value() += newSymbolCount;
    // insert the new symbols
    populateINameIndexMap(prefix, index, index + 1 + newSymbolCount);
    if (debug > 1)
        qDebug() << '<' << Q_FUNC_INFO << '\n' << prefix << index << '\n' << toString();
    return true;
}

void SymbolGroupContext::clear()
{
    m_symbolParameters.clear();
    m_inameIndexMap.clear();
}

QString SymbolGroupContext::symbolINameAt(unsigned long index) const
{
    return m_inameIndexMap.key(index);
}

// Return hexadecimal pointer value from a CDB pointer value
// which look like "0x000032a" or "0x00000000`0250124a" or
// "0x1`0250124a" on 64-bit systems.
bool SymbolGroupContext::getUnsignedHexValue(QString stringValue, quint64 *value)
{
    *value = 0;
    if (!stringValue.startsWith(QLatin1String("0x")))
        return false;
    stringValue.remove(0, 2);
    // Remove 64bit separator
    if (stringValue.size() > 9) {
        const int sepPos = stringValue.size() - 9;
        if (stringValue.at(sepPos) == QLatin1Char('`'))
            stringValue.remove(sepPos, 1);
    }
    bool ok;
    *value = stringValue.toULongLong(&ok, 16);
    return ok;
}

// check for "0x000", "0x000 class X" or its 64-bit equivalents.
bool SymbolGroupContext::isNullPointer(const QString &type , QString valueS)
{
    if (!type.endsWith(QLatin1String(" *")))
        return false;
    const int blankPos = valueS.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        valueS.truncate(blankPos);
    quint64 value;
    return SymbolGroupContext::getUnsignedHexValue(valueS, &value) && value == 0u;
}

// Fix a symbol group value. It is set to the class type for
// expandable classes (type="class std::foo<..>[*]",
// value="std::foo<...>[*]". This is not desired
// as it widens the value column for complex std::template types.
// Remove the inner template type.

QString SymbolGroupContext::removeInnerTemplateType(QString value)
{
    const int firstBracketPos = value.indexOf(QLatin1Char('<'));
    const int lastBracketPos = firstBracketPos != -1 ? value.lastIndexOf(QLatin1Char('>')) : -1;
    if (lastBracketPos != -1)
        value.replace(firstBracketPos + 1, lastBracketPos - firstBracketPos -1, QLatin1String("..."));
    return value;
}

QString SymbolGroupContext::formatShadowedName(const QString &name, int n) const
{
    return n > 0 ? m_shadowedNameFormat.arg(name).arg(n) : name;
}

unsigned SymbolGroupContext::dumpValueRaw(unsigned long index,
                                    QString *inameIn,
                                    QString *nameIn,
                                    ULONG64 *addrIn,
                                    ULONG *typeIdIn,
                                    QString *typeNameIn,
                                    QString *valueIn) const
{
    unsigned rc = 0;
    const QString iname = symbolINameAt(index);
    *inameIn = iname;
    *addrIn = symbolOffset(m_symbolGroup, index);
    // Determine name from iname and format shadowed variables correctly
    // as "<shadowed X>, see populateINameIndexMap() (from "name#1").
    const int lastDelimiterPos = iname.lastIndexOf(m_nameDelimiter);
    QString name = lastDelimiterPos == -1 ? iname : iname.mid(lastDelimiterPos + 1);
    int shadowedNumber = 0;
    const int shadowedPos = name.lastIndexOf(QLatin1Char(iNameShadowDelimiter));
    if (shadowedPos != -1) {
        shadowedNumber = name.mid(shadowedPos + 1).toInt();
        name.truncate(shadowedPos);
    }
    // For class hierarchies, we get sometimes complicated std::template types here.
    // (std::map extends std::tree<>... Remove them for display only.    
    *nameIn = formatShadowedName(removeInnerTemplateType(name), shadowedNumber);
    *typeNameIn = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolTypeNameWide, index);
    // Check for uninitialized variables at level 0 only.
    const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(index);
    *typeIdIn = p.TypeId;
    if (p.ParentSymbol == DEBUG_ANY_ID) {
        const QString fullShadowedName = formatShadowedName(name, shadowedNumber);
        if (m_uninitializedVariables.contains(fullShadowedName)) {
            rc |= OutOfScope;
            valueIn->clear();
            return rc;
        }
    }
    // In scope: Figure out value
    *valueIn = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolValueTextWide, index);
    // Figure out children. The SubElement is only a guess unless the symbol,
    // is expanded, so, we leave this as a guess for later updates.
    // If the symbol has children (expanded or not), we leave the 'Children' flag
    // in 'needed' state. Suppress 0-pointers right ("0x000 class X")
    // here as they only lead to children with memory access errors.
    if (p.SubElements && !isNullPointer(*typeNameIn, *valueIn))
        rc |= HasChildren;
    return rc;
}

/* The special type dumpers have an integer return code meaning:
 *  0:  ok
 *  1:  Dereferencing or retrieving memory failed, this is out of scope,
 *      do not try to query further.
 * > 1: A structural error was encountered, that is, the implementation
 *      of the class changed (Qt or say, a different STL implementation).
 *      Visibly warn about it.
 * To add further types, have a look at the toString() output of the
 * symbol group. */

static QString msgStructuralError(const QString &name, const QString &type, int code)
{
    return QString::fromLatin1("Warning: Internal dumper for '%1' (%2) failed with %3.").arg(name, type).arg(code);
}

static inline bool isStdStringOrPointer(const QString &type)
{
#define STD_WSTRING "std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >"
#define STD_STRING "std::basic_string<char,std::char_traits<char>,std::allocator<char> >"
    return type.endsWith(QLatin1String(STD_STRING))
            || type.endsWith(QLatin1String(STD_STRING" *"))
            || type.endsWith(QLatin1String(STD_WSTRING))
            || type.endsWith(QLatin1String(STD_WSTRING" *"));
#undef STD_WSTRING
#undef STD_STRING
}

unsigned SymbolGroupContext::dumpValue(unsigned long index,
                     QString *inameIn,
                     QString *nameIn,
                     ULONG64 *addrIn,
                     ULONG *typeIdIn,
                     QString *typeNameIn,
                     QString *valueIn)
{    
    unsigned rc = dumpValueRaw(index, inameIn, nameIn, addrIn, typeIdIn,
                         typeNameIn, valueIn);
    do {
        // Is this a previously detected Null-Pointer or out of scope
        if ( (rc & OutOfScope) || !(rc & HasChildren) )
            break;
        // QString
        if (typeNameIn->endsWith(QLatin1String("QString")) || typeNameIn->endsWith(QLatin1String("QString *"))) {
            const int drc = dumpQString(index, *inameIn, valueIn);
            switch (drc) {
            case 0:
                rc |= InternalDumperSucceeded;
                rc &= ~HasChildren;
                break;
            case 1:
                rc |= InternalDumperError;
                break;
            default:
                rc |= InternalDumperFailed;
                qWarning("%s\n", qPrintable(msgStructuralError(*inameIn, *typeNameIn, drc)));
                break;
            }
        }
        // StdString
        if (isStdStringOrPointer(*typeNameIn)) {
            const int drc = dumpStdString(index, *inameIn, valueIn);
            switch (drc) {
            case 0:
                rc |= InternalDumperSucceeded;
                rc &= ~HasChildren;
                break;
            case 1:
                rc |= InternalDumperError;
                break;
            default:
                rc |= InternalDumperFailed;
                qWarning("%s\n", qPrintable(msgStructuralError(*inameIn, *typeNameIn, drc)));
                break;
            }

        }
    } while (false);
    if (debugInternalDumpers) {
        QString msg;
        QTextStream str(&msg);
        str.setIntegerBase(16);
        str << "SymbolGroupContext::dump rc=0x" << rc;
        str.setIntegerBase(10);
         str << " Type='" << *typeNameIn;
        str << " (" << *typeIdIn << ") Name='" << *nameIn << "' Value='" << *valueIn << '\'';
        qDebug("%s", qPrintable(msg));
    }
    return rc;
}

// Get integer value of symbol group
bool SymbolGroupContext::getDecimalIntValue(CIDebugSymbolGroup *sg, int index, int *value)
{
    const QString valueS = getSymbolString(sg, &IDebugSymbolGroup2::GetSymbolValueTextWide, index);
    bool ok;
    *value = valueS.toInt(&ok);
    return ok;
}

// Get pointer value of symbol group ("0xAAB")
// Note that this is on "00000000`0250124a" on 64bit systems.
static inline bool getSG_UnsignedHexValue(CIDebugSymbolGroup *sg, int index, quint64 *value)
{
    const QString stringValue = getSymbolString(sg, &IDebugSymbolGroup2::GetSymbolValueTextWide, index);
    return SymbolGroupContext::getUnsignedHexValue(stringValue, value);
}

enum { maxStringLength = 4096 };

int SymbolGroupContext::dumpQString(unsigned long index,
                                    const QString &iname,
                                    QString *valueIn)
{
    valueIn->clear();
    QString errorMessage;
    // Expand string and it's "d" (step over 'static null')
    if (!expandSymbol(iname, index, &errorMessage))
        return 2;
    const unsigned long dIndex = index + 4;
    if (!expandSymbol(iname, dIndex, &errorMessage))
        return 3;
    const unsigned long sizeIndex = dIndex + 3;
    const unsigned long arrayIndex = dIndex + 4;
    // Get size and pointer
    int size;
    if (!getDecimalIntValue(m_symbolGroup, sizeIndex, &size))
        return 4;
    quint64 array;
    if (!getSG_UnsignedHexValue(m_symbolGroup, arrayIndex, &array))
        return 5;
    // Fetch
    const bool truncated = size > maxStringLength;
    if (truncated)
        size = maxStringLength;
    const QChar doubleQuote = QLatin1Char('"');
    if (size > 0) {
        valueIn->append(doubleQuote);
        // Should this ever be a remote debugger, need to check byte order.
        unsigned short *buf =  new unsigned short[size + 1];
        unsigned long bytesRead;
        const HRESULT hr = m_dataSpaces->ReadVirtual(array, buf, size * sizeof(unsigned short), &bytesRead);
        if (FAILED(hr)) {
            delete [] buf;
            return 1;
        }
        buf[bytesRead / sizeof(unsigned short)] = 0;
        valueIn->append(QString::fromUtf16(buf));
        delete [] buf;
        if (truncated)
            valueIn->append(QLatin1String("..."));
        valueIn->append(doubleQuote);
    } else if (size == 0) {
        *valueIn = QString(doubleQuote) + doubleQuote;
    } else {
        *valueIn = QLatin1String("Invalid QString");
    }
    return 0;
}

int SymbolGroupContext::dumpStdString(unsigned long index,
                                      const QString &inameIn,
                                      QString *valueIn)

{
    QString errorMessage;

    // Expand string ->string_val->_bx.
    if (!expandSymbol(inameIn,  index, &errorMessage))
        return 1;
    const unsigned long bxIndex =  index + 3;
    if (!expandSymbol(inameIn, bxIndex, &errorMessage))
        return 2;
    // Check if size is something sane
    const int sizeIndex =  index + 6;
    int size;
    if (!getDecimalIntValue(m_symbolGroup, sizeIndex, &size))
        return 3;
    if (size < 0)
        return 1;
    // Just copy over the value of the buf[]-array, which should be the string
    const QChar doubleQuote = QLatin1Char('"');
    const int bufIndex =  index + 4;
    *valueIn = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolValueTextWide, bufIndex);
    const int quotePos = valueIn->indexOf(doubleQuote);
    if (quotePos == -1)
        return 1;
    valueIn->remove(0, quotePos);
    if (valueIn->size() > maxStringLength) {
        valueIn->truncate(maxStringLength);
        valueIn->append(QLatin1String("...\""));
    }
    return 0;
}

bool SymbolGroupContext::assignValue(const QString &iname, const QString &value,
                                        QString *newValue, QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << iname << value;
    const NameIndexMap::const_iterator it = m_inameIndexMap.constFind(iname);
    if (it == m_inameIndexMap.constEnd()) {
        *errorMessage = msgSymbolNotFound(iname);
        return false;
    }
    const unsigned long  index = it.value();
    const HRESULT hr = m_symbolGroup->WriteSymbolWide(index, reinterpret_cast<PCWSTR>(value.utf16()));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to assign '%1' to '%2': %3").
                        arg(value, iname, CdbCore::msgComFailed("WriteSymbolWide", hr));
        return false;
    }
    if (newValue)
        *newValue = getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolValueTextWide, index);
    return true;
}

} // namespace CdbCore
