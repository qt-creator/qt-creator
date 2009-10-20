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
bool CdbSymbolGroupContext::getDumpChildSymbols(CIDebugDataSpaces *ds, const QString &prefix,
                             int dumpedOwner,
                             OutputIterator it, QString *errorMessage)
{
    unsigned long start;
    unsigned long parentId;
    if (!getChildSymbolsPosition(prefix, &start, &parentId, errorMessage))
        return false;
    // Skip over expanded children. Internal dumping might expand
    // children, so, re-evaluate size in end condition.
    for (int s = start; s < m_symbolParameters.size(); ++s) {
        const DEBUG_SYMBOL_PARAMETERS &p = m_symbolParameters.at(s);
        if (p.ParentSymbol == parentId && isSymbolDisplayable(p)) {
            WatchData wd = watchDataAt(s);
            // Run internal dumper, mark ownership
            if (ds) {
                switch (dump(ds, &wd)) {
                case DumperOk:
                case DumperError: // Not initialized yet, do not run other dumpers
                    wd.source = dumpedOwner;
                    break;
                case DumperNotHandled:
                    break;
                }
            }
            *it = wd;
            ++it;
        }
    }
    return true;
}

// Insert a symbol (and its first level children depending on forceRecursion)
// The parent symbol is inserted before the children to make dumper handling
// simpler. In theory, it can happen that the symbol context indicates
// children but can expand none, which would lead to invalid parent information
// (expand icon), though (ignore for simplicity).
template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
bool insertSymbolRecursion(WatchData wd,
                           const CdbSymbolGroupRecursionContext &ctx,
                           OutputIterator it,
                           RecursionPredicate recursionPredicate,
                           IgnorePredicate ignorePredicate,
                           QString *errorMessage)
{
    // Find out whether to recurse (has children and predicate agrees)
    const bool hasChildren = wd.hasChildren || wd.isChildrenNeeded();
    const bool recurse = hasChildren && recursionPredicate(wd);
    if (debugSgRecursion)
        qDebug() << "insertSymbolRecursion" << '\n' << wd.iname << "recurse=" << recurse;
    if (!recurse) {
        // No further recursion at this level, pretend entry is complete
        // to the watchmodel for the parent to show (only remaining watchhandler-specific
        // part here).
        if (wd.isChildrenNeeded()) {
            wd.setHasChildren(true);
            wd.setChildrenUnneeded();
        }
        if (debugSgRecursion)
            qDebug() << " INSERTING non-recursive: " << wd.toString();
        *it = wd;
        ++it;
        return true;
    }
    // Recursion: Indicate children unneeded
    wd.setHasChildren(true);
    wd.setChildrenUnneeded();
    if (debugSgRecursion)
        qDebug() << " INSERTING recursive: " << wd.toString();
    *it = wd;
    ++it;
    // Recurse unless the predicate disagrees
    if (ignorePredicate(wd))
        return true;
    QList<WatchData> watchList;
    // This implicitly enforces expansion
    if (!ctx.context->getDumpChildSymbols(ctx.dataspaces,
                                          wd.iname,
                                          ctx.internalDumperOwner,
                                          WatchDataBackInserter(watchList), errorMessage))
        return false;
    const int childCount = watchList.size();
    for (int c = 0; c < childCount; c++) {
        const WatchData &cwd = watchList.at(c);
        if (wd.isValid()) { // We sometimes get empty names for deeply nested data
            if (!insertSymbolRecursion(cwd, ctx, it, recursionPredicate, ignorePredicate, errorMessage))
                return false;
        }  else {
            const QString msg = QString::fromLatin1("WARNING: Skipping invalid child symbol #%2 (type %3) of '%4'.").
                                arg(QLatin1String(Q_FUNC_INFO)).arg(c).arg(cwd.type, wd.iname);
            qWarning("%s\n", qPrintable(msg));
        }
    }
    return true;
}

template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
bool CdbSymbolGroupContext::populateModelInitially(const CdbSymbolGroupRecursionContext &ctx,
                                                   OutputIterator it,
                                                   RecursionPredicate recursionPredicate,
                                                   IgnorePredicate ignorePredicate,
                                                   QString *errorMessage)
{
    if (debugSgRecursion)
        qDebug() << "### CdbSymbolGroupContext::populateModelInitially";

    // Insert root items
    QList<WatchData> watchList;
    CdbSymbolGroupContext *sg = ctx.context;
    if (!sg->getDumpChildSymbols(ctx.dataspaces, sg->prefix(),
                                 ctx.internalDumperOwner,
                                 WatchDataBackInserter(watchList), errorMessage))
        return false;
    // Insert data
    foreach(const WatchData &wd, watchList)
        if (!insertSymbolRecursion(wd, ctx, it, recursionPredicate, ignorePredicate, errorMessage))
            return false;
    return true;
}

template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
bool CdbSymbolGroupContext::completeData(const CdbSymbolGroupRecursionContext &ctx,
                                         WatchData incompleteLocal,
                                         OutputIterator it,
                                         RecursionPredicate recursionPredicate,
                                         IgnorePredicate ignorePredicate,
                                         QString *errorMessage)
{
    if (debugSgRecursion)
        qDebug().nospace() << "###>CdbSymbolGroupContext::completeData" << ' ' << incompleteLocal.iname << '\n';
    // If the symbols are already expanded in the context, they will be re-inserted,
    // which is not handled for simplicity.
    if (!insertSymbolRecursion(incompleteLocal, ctx, it, recursionPredicate, ignorePredicate, errorMessage))
        return false;
    return true;
}

} // namespace Internal
} // namespace Debugger

#endif // CDBSYMBOLGROUPCONTEXT_TPL_H
