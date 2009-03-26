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

#ifndef CDBSYMBOLGROUPCONTEXT_H
#define CDBSYMBOLGROUPCONTEXT_H

#include <windows.h>
#include <inc/dbgeng.h>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QPair>
#include <QtCore/QMap>

namespace Debugger {
    namespace Internal {

class WatchData;

/* A thin wrapper around the IDebugSymbolGroup2 interface which represents
 * a flat list of symbols using an index (for example, belonging to a stack frame).
 * It uses the hierarchical naming convention of WatchHandler as:
 * "local" (invisible root)
 * "local.string" (local class variable)
 * "local.string.data" (class member).
 * IDebugSymbolGroup2 can "expand" expandable symbols, appending to the flat list.
 */

class CdbSymbolGroupContext
{
    Q_DISABLE_COPY(CdbSymbolGroupContext);

    // Start position and length of range in m_symbolParameters
    typedef QPair<unsigned long, unsigned long> Range;

public:
    explicit CdbSymbolGroupContext(const QString &prefix,
                                   IDebugSymbolGroup2 *symbolGroup);
    ~CdbSymbolGroupContext();

    QString prefix() const { return m_prefix; }

    // Retrieve child symbols of prefix as a sequence of WatchData.
    template <class OutputIterator>
            void getSymbols(const QString &prefix, OutputIterator it);

private:
    void clear();
    Range getSymbolRange(const QString &prefix);
    Range allocateChildSymbols(const QString &prefix);
    Range allocateRootSymbols();
    WatchData symbolAt(unsigned long index) const;

    inline DEBUG_SYMBOL_PARAMETERS *symbolParameters() { return &(*m_symbolParameters.begin()); }
    inline const DEBUG_SYMBOL_PARAMETERS *symbolParameters() const { return &(*m_symbolParameters.constBegin()); }

    const QString m_prefix;
    const QChar m_nameDelimiter;
    IDebugSymbolGroup2 *m_symbolGroup;

    QStringList m_symbolINames;
    QVector<DEBUG_SYMBOL_PARAMETERS> m_symbolParameters;

    typedef QMap<QString, Range> ChildRangeMap;

    ChildRangeMap m_childRanges;
};

template <class OutputIterator>
void CdbSymbolGroupContext::getSymbols(const QString &prefix, OutputIterator it)
{
    const Range  r = getSymbolRange(prefix);
    const unsigned long end = r.first + r.second;
    for (unsigned long i = r.first; i < end; i++) {
        *it = symbolAt(i);
        ++it;
    }
}

}
}
#endif // CDBSYMBOLGROUPCONTEXT_H
