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

#ifndef CDBSYMBOLGROUPCONTEXT_H
#define CDBSYMBOLGROUPCONTEXT_H

#include "cdbcom.h"
#include "symbolgroupcontext.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QPair>
#include <QtCore/QMap>
#include <QtCore/QSet>

namespace Debugger {
namespace Internal {

class WatchData;
class WatchHandler;
struct CdbSymbolGroupRecursionContext;
class CdbDumperHelper;


/* CdbSymbolGroupContext manages a symbol group context and
 * a dumper context. It dispatches calls between the local items
 * that are handled by the symbol group and those that are handled by the dumpers. */

class CdbSymbolGroupContext : public CdbCore::SymbolGroupContext
{
    Q_DISABLE_COPY(CdbSymbolGroupContext);
     explicit CdbSymbolGroupContext(const QString &prefix,
                                    CIDebugSymbolGroup *symbolGroup,
                                    const QSharedPointer<CdbDumperHelper> &dumper,
                                    const QStringList &uninitializedVariables);

public:
     // Mask bits for the source field of watch data.
     enum { SourceMask = 0xFF, ChildrenKnownBit = 0x0100 };

     static CdbSymbolGroupContext *create(const QString &prefix,
                                          CIDebugSymbolGroup *symbolGroup,
                                          const QSharedPointer<CdbDumperHelper> &dumper,
                                          const QStringList &uninitializedVariables,
                                          QString *errorMessage);

     bool editorToolTip(const QString &iname, QString *value, QString *errorMessage);

     bool populateModelInitially(WatchHandler *wh, QString *errorMessage);

     bool completeData(const WatchData &incompleteLocal,
                       WatchHandler *wh,
                       QString *errorMessage);     

private:
    // Initially populate the locals model for a new stackframe.
    // Write a sequence of WatchData to it, recurse down if the
    // recursionPredicate agrees. The ignorePredicate can be used
    // to terminate processing after insertion of an item (if the calling
    // routine wants to insert another subtree).
    template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
    static bool populateModelInitiallyHelper(const CdbSymbolGroupRecursionContext &ctx,
                                       OutputIterator it,
                                       RecursionPredicate recursionPredicate,
                                       IgnorePredicate ignorePredicate,
                                       QString *errorMessage);

    // Complete children of a WatchData item.
    // Write a sequence of WatchData to it, recurse if the
    // recursionPredicate agrees. The ignorePredicate can be used
    // to terminate processing after insertion of an item (if the calling
    // routine wants to insert another subtree).
    template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
    static bool completeDataHelper (const CdbSymbolGroupRecursionContext &ctx,
                              WatchData incompleteLocal,
                              OutputIterator it,
                              RecursionPredicate recursionPredicate,
                              IgnorePredicate ignorePredicate,
                              QString *errorMessage);

    // Retrieve child symbols of prefix as a sequence of WatchData.
    template <class OutputIterator>
            bool getChildSymbols(const QString &prefix, OutputIterator it, QString *errorMessage)
            { return getDumpChildSymbols(0, prefix, 0, it, errorMessage); }
    // Retrieve child symbols of prefix as a sequence of WatchData.
    // Is CIDebugDataSpaces is != 0, try internal dumper and set owner
    template <class OutputIterator>
            bool getDumpChildSymbols(const QString &prefix,
                                     int dumpedOwner,
                                     OutputIterator it, QString *errorMessage);

    template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
    static bool insertSymbolRecursion(WatchData wd,
                                      const CdbSymbolGroupRecursionContext &ctx,
                                      OutputIterator it,
                                      RecursionPredicate recursionPredicate,
                                      IgnorePredicate ignorePredicate,
                                      QString *errorMessage);

    unsigned watchDataAt(unsigned long index, WatchData *);

    const bool m_useDumpers;
    const QSharedPointer<CdbDumperHelper> m_dumper;
};

// A convenience struct to save parameters for the model recursion.
struct CdbSymbolGroupRecursionContext {
    explicit CdbSymbolGroupRecursionContext(CdbSymbolGroupContext *ctx, int internalDumperOwner);

    CdbSymbolGroupContext *context;
    int internalDumperOwner;
};

// Helper to a sequence of  WatchData into a list.
class WatchDataBackInserter
{
public:
    explicit WatchDataBackInserter(QList<WatchData> &wh) : m_wh(wh) {}

    inline WatchDataBackInserter & operator*() { return *this; }
    inline WatchDataBackInserter &operator=(const WatchData &wd) {
        m_wh.push_back(wd);
        return *this;
    }
    inline WatchDataBackInserter &operator++() { return *this; }

private:
    QList<WatchData> &m_wh;
};

} // namespace Internal
} // namespace Debugger

#include "cdbsymbolgroupcontext_tpl.h"

#endif // CDBSYMBOLGROUPCONTEXT_H
