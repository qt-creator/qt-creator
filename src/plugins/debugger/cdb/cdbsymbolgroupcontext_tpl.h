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

#ifndef CDBSYMBOLGROUPCONTEXT_TPL_H
#define CDBSYMBOLGROUPCONTEXT_TPL_H

#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

enum { debugSgRecursion = 0 };

/* inline static */ bool CdbSymbolGroupContext::isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.Flags & (DEBUG_SYMBOL_IS_LOCAL|DEBUG_SYMBOL_IS_ARGUMENT))
        return true;
    // Do not display static members.
    if (p.Flags & DEBUG_SYMBOL_READ_ONLY)
        return false;
    return true;
}

template <class OutputIterator>
bool CdbSymbolGroupContext::getChildSymbols(const QString &prefix, OutputIterator it, QString *errorMessage)
{
    unsigned long start;
    unsigned long parentId;
    if (!getChildSymbolsPosition(prefix, &start, &parentId, errorMessage))
        return false;
    // Skip over expanded children
    const unsigned long end = m_symbolParameters.size();
    for (unsigned long s = start; s < end; ++s) {
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(s);
        if (p.ParentSymbol == parentId && isSymbolDisplayable(p)) {
            *it = symbolAt(s);
            ++it;
        }
    }
    return true;
}

template <class OutputIterator>
        bool insertChildrenRecursion(const QString &iname,
                                     CdbSymbolGroupContext *sg,
                                     OutputIterator it,
                                     int maxRecursionLevel,
                                     int level,
                                     QString *errorMessage,
                                     int *childCount = 0);

// Insert a symbol (and its first level children depending on forceRecursion)
template <class OutputIterator>
bool insertSymbolRecursion(WatchData wd,
                                  CdbSymbolGroupContext *sg,
                                  OutputIterator it,
                                  int maxRecursionLevel,
                                  int level,
                                  QString *errorMessage)
{
    // Find out whether to recurse (has children or at least knows it has children)
    // Open next level if specified by recursion depth or child is already expanded
    // (Sometimes, some root children are already expanded after creating the context).
    const bool hasChildren = wd.childCount > 0 || wd.isChildrenNeeded();
    const bool recurse = hasChildren && (level < maxRecursionLevel || sg->isExpanded(wd.iname));
    if (debugSgRecursion)
        qDebug() << Q_FUNC_INFO << '\n' << wd.iname << "level=" << level <<  "recurse=" << recurse;
    bool rc = true;
    if (recurse) { // Determine number of children and indicate in model
        int childCount;
        rc = insertChildrenRecursion(wd.iname, sg, it, maxRecursionLevel, level, errorMessage, &childCount);
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
    if (debugSgRecursion)
        qDebug() << " INSERTING: at " << level << wd.toString();
    *it = wd;
    ++it;
    return rc;
}

// Insert the children of prefix.
template <class OutputIterator>
        bool insertChildrenRecursion(const QString &iname,
                                     CdbSymbolGroupContext *sg,
                                     OutputIterator it,
                                     int maxRecursionLevel,
                                     int level,
                                     QString *errorMessage,
                                     int *childCountPtr)
{
    if (debugSgRecursion > 1)
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
            if (!insertSymbolRecursion(wd, sg, it, maxRecursionLevel, level + 1, errorMessage))
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

template <class OutputIterator>
bool CdbSymbolGroupContext::populateModelInitially(CdbSymbolGroupContext *sg,
                                                   OutputIterator it,
                                                   QString *errorMessage)
{
    if (debugSgRecursion)
        qDebug() << "###" << Q_FUNC_INFO;

    // Insert root items
    QList<WatchData> watchList;
    if (!sg->getChildSymbols(sg->prefix(), WatchDataBackInserter(watchList), errorMessage))
        return false;

    foreach(const WatchData &wd, watchList)
        if (!insertSymbolRecursion(wd, sg, it, 0, 0, errorMessage))
            return false;
    return true;
}

template <class OutputIterator>
bool CdbSymbolGroupContext::completeModel(CdbSymbolGroupContext *sg,
                                          const QList<WatchData> &incompleteLocals,
                                          OutputIterator it,
                                          QString *errorMessage)
{
    if (debugSgRecursion)
        qDebug().nospace() << "###>" << Q_FUNC_INFO << ' ' << incompleteLocals.size() << '\n';
    // The view reinserts any node being expanded with flag 'ChildrenNeeded'.
    // Recurse down one level in context unless this is already the case.
    foreach(WatchData wd, incompleteLocals) {
        const bool contextExpanded = sg->isExpanded(wd.iname);
        if (debugSgRecursion)
            qDebug() << "  " << wd.iname << "CE=" << contextExpanded;
        if (contextExpanded) { // You know that already.
            wd.setChildrenUnneeded();            
            *it = wd;
            ++it;
        } else {
            if (!insertSymbolRecursion(wd, sg, it, 1, 0, errorMessage))
                return false;
        }
    }
    return true;
}

} // namespace Internal
} // namespace Debugger

#endif // CDBSYMBOLGROUPCONTEXT_TPL_H
