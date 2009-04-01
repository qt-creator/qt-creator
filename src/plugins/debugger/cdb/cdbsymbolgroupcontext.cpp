/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbsymbolgroupcontext.h"
#include "cdbdebugengine_p.h"
#include "watchhandler.h"
#include "watchutils.h"
#include <QtCore/QTextStream>

enum { maxRecursionDepth = 10 };

static inline bool isTopLevelSymbol(const DEBUG_SYMBOL_PARAMETERS &p) { return p.ParentSymbol == DEBUG_ANY_ID; }

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

 QTextStream &operator<<(QTextStream &str, const DEBUG_SYMBOL_PARAMETERS& p)
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

// A helper function to extract a string value from a member function of
// IDebugSymbolGroup2 taking the symbol index and a character buffer.
// Pass in the the member function as '&IDebugSymbolGroup2::GetSymbolNameWide'

typedef HRESULT  (__stdcall IDebugSymbolGroup2::*WideStringRetrievalFunction)(ULONG, PWSTR, ULONG, PULONG);

static inline QString getSymbolString(IDebugSymbolGroup2 *sg,
                                      WideStringRetrievalFunction wsf,
                                      unsigned long index)
{
    static WCHAR nameBuffer[MAX_PATH + 1];
    // Name
    ULONG nameLength;
    const HRESULT hr = (sg->*wsf)(index, nameBuffer, MAX_PATH, &nameLength);
    if (SUCCEEDED(hr)) {
        nameBuffer[nameLength] = 0;
        return QString::fromUtf16(nameBuffer);
    }
    return QString();
}

namespace Debugger {
    namespace Internal {

static inline CdbSymbolGroupContext::SymbolState getSymbolState(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.SubElements == 0u)
        return CdbSymbolGroupContext::LeafSymbol;
    return (p.Flags & DEBUG_SYMBOL_EXPANDED) ?
               CdbSymbolGroupContext::ExpandedSymbol :
               CdbSymbolGroupContext::CollapsedSymbol;
}

CdbSymbolGroupContext::CdbSymbolGroupContext(const QString &prefix,
                                             IDebugSymbolGroup2 *symbolGroup) :
    m_prefix(prefix),
    m_nameDelimiter(QLatin1Char('.')),
    m_symbolGroup(symbolGroup)
{
}

CdbSymbolGroupContext::~CdbSymbolGroupContext()
{
    m_symbolGroup->Release();
}

CdbSymbolGroupContext *CdbSymbolGroupContext::create(const QString &prefix,
                                                     IDebugSymbolGroup2 *symbolGroup,
                                                     QString *errorMessage)
{
    CdbSymbolGroupContext *rc= new CdbSymbolGroupContext(prefix, symbolGroup);
    if (!rc->init(errorMessage)) {
        delete rc;
        return 0;
    }
    return rc;
}

bool CdbSymbolGroupContext::init(QString *errorMessage)
{
    // retrieve the root symbols
    ULONG count;
    HRESULT hr = m_symbolGroup->GetNumberSymbols(&count);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetNumberSymbols", hr);
        return false;
    }

    m_symbolParameters.reserve(3u * count);
    m_symbolParameters.resize(count);

    hr = m_symbolGroup->GetSymbolParameters(0, count, symbolParameters());
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetSymbolParameters", hr);
        return false;
    }
    populateINameIndexMap(m_prefix, DEBUG_ANY_ID, 0, count);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n'<< toString();
    return true;
}

void CdbSymbolGroupContext::populateINameIndexMap(const QString &prefix, unsigned long parentId,
                                                  unsigned long start, unsigned long count)
{
    // Make the entries for iname->index mapping. We might encounter
    // already expanded subitems when doing it for top-level, recurse in that case.
    const QString symbolPrefix = prefix + m_nameDelimiter;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n'<< symbolPrefix << start << count;
    const unsigned long end = m_symbolParameters.size();
    unsigned long seenChildren = 0;
    // Skip over expanded children
    for (unsigned long i = start; i < end && seenChildren < count; i++) {
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(i);
        if (parentId == p.ParentSymbol) {
            seenChildren++;
            const QString name = symbolPrefix + getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolNameWide, i);
            m_inameIndexMap.insert(name, i);
            if (getSymbolState(p) == ExpandedSymbol)
                populateINameIndexMap(name, i, i + 1, p.SubElements);
        }
    }
}

QString CdbSymbolGroupContext::toString(bool verbose) const
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
            str << " '" << getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolTypeNameWide, i);
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

bool CdbSymbolGroupContext::isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.Flags & (DEBUG_SYMBOL_IS_LOCAL|DEBUG_SYMBOL_IS_ARGUMENT))
        return true;
    // Do not display static members.
    if (p.Flags & DEBUG_SYMBOL_READ_ONLY)
        return false;
    return true;
}

CdbSymbolGroupContext::SymbolState CdbSymbolGroupContext::symbolState(unsigned long index) const
{
    return getSymbolState(m_symbolParameters.at(index));
}

CdbSymbolGroupContext::SymbolState CdbSymbolGroupContext::symbolState(const QString &prefix) const
{
    if (prefix == m_prefix) // root
        return ExpandedSymbol;
    const NameIndexMap::const_iterator it = m_inameIndexMap.constFind(prefix);
    if (it != m_inameIndexMap.constEnd())
        return symbolState(it.value());
    qWarning("WARNING %s: %s not found.\n", Q_FUNC_INFO, qPrintable(prefix));
    return LeafSymbol;
}

/* Retrieve children and get the position. */
bool CdbSymbolGroupContext::getChildSymbolsPosition(const QString &prefix,
                                                    unsigned long *start,
                                                    unsigned long *parentId,
                                                    QString *errorMessage)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n'<< prefix;

    *start = *parentId = 0;
    // Root item?
    if (prefix == m_prefix) {
        *start = 0;
        *parentId = DEBUG_ANY_ID;
        if (debugCDB)
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
    if (debugCDB)
        qDebug() << '<' << prefix << "at" << *start;
    return true;
}

// Expand a symbol using the symbol group interface.
bool CdbSymbolGroupContext::expandSymbol(const QString &prefix, unsigned long index, QString *errorMessage)
{
    if (debugCDB)
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
        *errorMessage = QString::fromLatin1("Unable to expand '%1' %2: %3").
                        arg(prefix).arg(index).arg(msgComFailed("ExpandSymbol", hr));
        return false;
    }
    // Hopefully, this will never fail, else data structure will be foobar.
    const ULONG oldSize = m_symbolParameters.size();
    ULONG newSize;
    hr = m_symbolGroup->GetNumberSymbols(&newSize);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetNumberSymbols", hr);
        return false;
    }

    // Retrieve the new parameter structs which will be inserted
    // after the parents, offsetting consecutive indexes.
    m_symbolParameters.resize(newSize);

    hr = m_symbolGroup->GetSymbolParameters(0, newSize, symbolParameters());
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetSymbolParameters", hr);
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
    populateINameIndexMap(prefix, index, index + 1, newSymbolCount);
    if (debugCDB > 1)
        qDebug() << '<' << Q_FUNC_INFO << '\n' << prefix << index << '\n' << toString();
    return true;
}

void CdbSymbolGroupContext::clear()
{
    m_symbolParameters.clear();
    m_inameIndexMap.clear();
}

// Expand all top level symbols
bool CdbSymbolGroupContext::expandTopLevel(QString *errorMessage)
{
    if (debugCDB)
        qDebug() << '>' << Q_FUNC_INFO;

    for (int i = m_symbolParameters.size() - 1; i >= 0; i --) {
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(i);
        if (isTopLevelSymbol(p) && (getSymbolState(p) == CollapsedSymbol) && isSymbolDisplayable(p))
            if (!expandSymbol(symbolINameAt(i), i, errorMessage))
                return false;
    }
    if (debugCDB)
        qDebug() << '<' << Q_FUNC_INFO << '\n' << toString();
    return true;
}

int CdbSymbolGroupContext::getDisplayableChildCount(unsigned long index) const
{
    if (!isExpanded(index))
        return 0;
    int rc = 0;
    // Skip over expanded children, count displayable ones
    const unsigned long childCount = m_symbolParameters.at(index).SubElements;
    unsigned long seenChildren = 0;
    for (unsigned long c = index + 1; seenChildren < childCount; c++) {
        const DEBUG_SYMBOL_PARAMETERS &params = m_symbolParameters.at(c);
        if (params.ParentSymbol == index) {
            seenChildren++;
            if (isSymbolDisplayable(params))
                rc++;
        }
    }
    return rc;
}

QString CdbSymbolGroupContext::symbolINameAt(unsigned long index) const
{
    return m_inameIndexMap.key(index);
}

WatchData CdbSymbolGroupContext::symbolAt(unsigned long index) const
{
    WatchData wd;
    wd.iname = symbolINameAt(index);
    const int lastDelimiterPos = wd.iname.lastIndexOf(m_nameDelimiter);
    wd.name = lastDelimiterPos == -1 ? wd.iname : wd.iname.mid(lastDelimiterPos + 1);
    wd.setType(getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolTypeNameWide, index));
    wd.setValue(getSymbolString(m_symbolGroup, &IDebugSymbolGroup2::GetSymbolValueTextWide, index).toUtf8());
    wd.setChildrenNeeded(); // compensate side effects of above setters
    wd.setChildCountNeeded();
    // Figure out children. The SubElement is only a guess unless the symbol,
    // is expanded, so, we leave this as a guess for later updates.
    // If the symbol has children (expanded or not), we leave the 'Children' flag
    // in 'needed' state.
    const DEBUG_SYMBOL_PARAMETERS &params = m_symbolParameters.at(index);
    if (params.SubElements) {
        if (getSymbolState(params) == ExpandedSymbol)
            wd.setChildCount(getDisplayableChildCount(index));
    } else {
        wd.setChildCount(0);
    }
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << index << '\n' << wd.toString();
    return wd;
}

class WatchDataBackInserter {
public:
    explicit WatchDataBackInserter(QList<WatchData> &wh) : m_wh(wh) {}

    WatchDataBackInserter & operator*() { return *this; }
    WatchDataBackInserter &operator=(const WatchData &wd) {
        m_wh.push_back(wd);
        return *this;
    }
    WatchDataBackInserter &operator++() { return *this; }

private:
    QList<WatchData> &m_wh;
};

static bool insertChildrenRecursion(const QString &iname,
                                    CdbSymbolGroupContext *sg,
                                    WatchHandler *watchHandler,
                                    bool forceRecursion,
                                    int level,
                                    QString *errorMessage,
                                    int *childCount = 0);

// Insert a symbol (and its first level children depending on forceRecursion)
static bool insertSymbolRecursion(WatchData wd,
                                  CdbSymbolGroupContext *sg,
                                  WatchHandler *watchHandler,
                                  bool forceRecursion,
                                  int level,
                                  QString *errorMessage)
{    
    if (level > maxRecursionDepth) {
        *errorMessage = QString::fromLatin1("Max recursion level %1 reached for '%2', bailing out.").arg(level).arg(wd.iname);
        return false;
    }
    // Find out whether to recurse (has children or at least knows it has children)
    const bool recurse = forceRecursion && (wd.childCount > 0 || wd.isChildrenNeeded());
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n' << wd.iname << "level=" << level <<  "recurse=" << recurse;
    bool rc = true;
    if (recurse) { // Determine number of children and indicate in model
        int childCount;
        rc = insertChildrenRecursion(wd.iname, sg, watchHandler, false, level, errorMessage, &childCount);
        if (rc) {
            wd.setChildCount(childCount);
            wd.setChildrenUnneeded();
        }
    } else {
        // No further recursion at this level, pretend entry is complete
        if (wd.isChildrenNeeded()) {
            wd.setChildCount(1);
            wd.setChildrenUnneeded();
        }
    }
    if (debugCDB)
        qDebug() << " INSERTING: at " << level << wd.toString();
    watchHandler->insertData(wd);
    return rc;
}

// Insert the children of prefix.
static bool insertChildrenRecursion(const QString &iname,
                                    CdbSymbolGroupContext *sg,
                                    WatchHandler *watchHandler,
                                    bool forceRecursion,
                                    int level,
                                    QString *errorMessage,
                                    int *childCountPtr)
{
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << '\n' << iname << level;

    QList<WatchData> watchList;
    // This implicitly enforces expansion
    if (!sg->getChildSymbols(iname, WatchDataBackInserter(watchList), errorMessage))
        return false;

    const int childCount = watchList.size();
    if (childCountPtr)
        *childCountPtr = childCount;
    int succeededChildCount = 0;
    for (int c = 0; c < childCount; c++) {
        const WatchData &wd = watchList.at(c);
        if (wd.isValid()) { // We sometimes get empty names for deeply nested data
            if (!insertSymbolRecursion(wd, sg, watchHandler, forceRecursion, level + 1, errorMessage))
                return false;
            succeededChildCount++;
        }  else {
            const QString msg = QString::fromLatin1("WARNING: Skipping invalid child symbol #%2 (type %3) of '%4'.").
                                arg(QLatin1String(Q_FUNC_INFO)).arg(c).arg(wd.type, iname);
            qWarning("%s\n", qPrintable(msg));
        }
    }
    if (childCountPtr)
        *childCountPtr = succeededChildCount;
    return true;
}

bool CdbSymbolGroupContext::populateModelInitially(CdbSymbolGroupContext *sg,
                                                   WatchHandler *watchHandler,
                                                   QString *errorMessage)
{
    if (debugCDB)
        qDebug() << "###" << Q_FUNC_INFO;

    if (!sg->expandTopLevel(errorMessage))
        return false;

    // Insert root items and known children of level 1
    QList<WatchData> watchList;
    if (!sg->getChildSymbols(sg->prefix(), WatchDataBackInserter(watchList), errorMessage))
        return false;

    foreach(const WatchData &wd, watchList)
        if (!insertSymbolRecursion(wd, sg, watchHandler, true, 0, errorMessage))
            return false;
    return true;
}

bool CdbSymbolGroupContext::completeModel(CdbSymbolGroupContext *sg,
                                          WatchHandler *watchHandler,
                                          QString *errorMessage)
{
    const QList<WatchData> incomplete = watchHandler->takeCurrentIncompletes();
    if (debugCDB)
        qDebug().nospace() << "###>" << Q_FUNC_INFO << ' ' << incomplete.size() << '\n';
    // The view reinserts any node being expanded with flag 'ChildrenNeeded'.
    // Expand next level in context unless this is already the case.
    foreach(WatchData wd, incomplete) {        
        const bool contextExpanded = sg->isExpanded(wd.iname);
        if (debugCDB)
            qDebug() << "  " << wd.iname << "CE=" << contextExpanded;
        if (contextExpanded) { // You know that already.
            wd.setChildrenUnneeded();
            watchHandler->insertData(wd);
        } else {
            if (!insertSymbolRecursion(wd, sg, watchHandler, true, 0, errorMessage))
                return false;
        }
    }
    watchHandler->rebuildModel();
    return true;
}

}
}
