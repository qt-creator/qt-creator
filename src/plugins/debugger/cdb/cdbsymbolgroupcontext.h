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
class WatchHandler;

/* A thin wrapper around the IDebugSymbolGroup2 interface which represents
 * a flat list of symbols using an index (for example, belonging to a stack
 * frame). It uses the hierarchical naming convention of WatchHandler as in:
 * "local" (invisible root)
 * "local.string" (local class variable)
 * "local.string.data" (class member)
 * and maintains a mapping iname -> index.
 * IDebugSymbolGroup2 can "expand" expandable symbols, inserting them into the
 * flat list after their parent. */

class CdbSymbolGroupContext
{
    Q_DISABLE_COPY(CdbSymbolGroupContext);
     explicit CdbSymbolGroupContext(const QString &prefix,
                                    IDebugSymbolGroup2 *symbolGroup);

public:
    ~CdbSymbolGroupContext();
    static CdbSymbolGroupContext *create(const QString &prefix,
                                         IDebugSymbolGroup2 *symbolGroup,
                                         QString *errorMessage);

    QString prefix() const { return m_prefix; }

    static bool populateModelInitially(CdbSymbolGroupContext *sg, WatchHandler *wh, QString *errorMessage);
    static bool completeModel(CdbSymbolGroupContext *sg, WatchHandler *wh, QString *errorMessage);

    // Retrieve child symbols of prefix as a sequence of WatchData.
    template <class OutputIterator>
            bool getChildSymbols(const QString &prefix, OutputIterator it, QString *errorMessage);

    bool expandTopLevel(QString *errorMessage);

    enum SymbolState { LeafSymbol, ExpandedSymbol, CollapsedSymbol };
    SymbolState symbolState(unsigned long index) const;
    SymbolState symbolState(const QString &prefix) const;

    inline bool isExpanded(unsigned long index) const   { return symbolState(index) == ExpandedSymbol; }
    inline bool isExpanded(const QString &prefix) const { return symbolState(prefix) == ExpandedSymbol; }

private:
    typedef QMap<QString, unsigned long>  NameIndexMap;

    static inline bool isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p);

    bool init(QString *errorMessage);
    void clear();
    QString toString() const;
    bool getChildSymbolsPosition(const QString &prefix,
                                 unsigned long *startPos,
                                 unsigned long *parentId,
                                 QString *errorMessage);
    bool expandSymbol(const QString &prefix, unsigned long index, QString *errorMessage);
    void populateINameIndexMap(const QString &prefix, unsigned long parentId, unsigned long start, unsigned long count);
    WatchData symbolAt(unsigned long index) const;
    QString symbolINameAt(unsigned long index) const;
    int getDisplayableChildCount(unsigned long index) const;

    inline DEBUG_SYMBOL_PARAMETERS *symbolParameters() { return &(*m_symbolParameters.begin()); }
    inline const DEBUG_SYMBOL_PARAMETERS *symbolParameters() const { return &(*m_symbolParameters.constBegin()); }

    const QString m_prefix;
    const QChar m_nameDelimiter;

    IDebugSymbolGroup2 *m_symbolGroup;
    NameIndexMap m_inameIndexMap;
    QVector<DEBUG_SYMBOL_PARAMETERS> m_symbolParameters;
};

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

}
}
#endif // CDBSYMBOLGROUPCONTEXT_H
