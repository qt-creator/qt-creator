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

#include "cdbstackframecontext.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdumperhelper.h"
#include "debuggeractions.h"
#include "watchhandler.h"

#include <QtCore/QDebug>

enum { debug = 0 };

namespace Debugger {
namespace Internal {

enum { OwnerNewItem, OwnerSymbolGroup, OwnerDumper };

typedef QSharedPointer<CdbDumperHelper> SharedPointerCdbDumperHelper;
typedef QList<WatchData> WatchDataList;

// Put a sequence of  WatchData into the model
class WatchHandlerModelInserter {
public:
    explicit WatchHandlerModelInserter(WatchHandler *wh) : m_wh(wh) {}

    inline WatchHandlerModelInserter & operator*() { return *this; }
    inline WatchHandlerModelInserter &operator=(const WatchData &wd)  {
        m_wh->insertData(wd);
        return *this;
    }
    inline WatchHandlerModelInserter &operator++() { return *this; }

private:
    WatchHandler *m_wh;
};

// Helper to sort apart a sequence of  WatchData using the Dumpers.
// Puts the stuff for which there is no dumper in the model
// as is and sets ownership to symbol group. The rest goes
// to the dumpers.
class WatchHandlerSorterInserter {
public:
    explicit WatchHandlerSorterInserter(WatchHandler *wh,
                                        const SharedPointerCdbDumperHelper &dumper);

    inline WatchHandlerSorterInserter & operator*() { return *this; }
    inline WatchHandlerSorterInserter &operator=(WatchData wd);
    inline WatchHandlerSorterInserter &operator++() { return *this; }

private:
    WatchHandler *m_wh;
    const SharedPointerCdbDumperHelper m_dumper;
    QList<WatchData> m_dumperResult;
    QStringList m_dumperINames;
};

WatchHandlerSorterInserter::WatchHandlerSorterInserter(WatchHandler *wh,
                                                       const SharedPointerCdbDumperHelper &dumper) :
    m_wh(wh),
    m_dumper(dumper)
{
}

WatchHandlerSorterInserter &WatchHandlerSorterInserter::operator=(WatchData wd)
{
    // Is this a child belonging to some item handled by dumpers,
    // such as d-elements of QStrings -> just ignore it.
    if (const int dumperINamesCount = m_dumperINames.size()) {
        for (int i = 0; i < dumperINamesCount; i++)
            if (wd.iname.startsWith(m_dumperINames.at(i)))
                return *this;
    }
    QString errorMessage;
    switch (m_dumper->dumpType(wd, true, OwnerDumper, &m_dumperResult, &errorMessage)) {
    case CdbDumperHelper::DumpOk:
        // Discard the original item and insert the dumper results
        m_dumperINames.push_back(wd.iname + QLatin1Char('.'));
        foreach(const WatchData &dwd, m_dumperResult)
            m_wh->insertData(dwd);
        break;
    case CdbDumperHelper::DumpNotHandled:
    case CdbDumperHelper::DumpError:
        wd.source = OwnerSymbolGroup;
        m_wh->insertData(wd);
        break;
    }
    return *this;
}

// -----------CdbStackFrameContext
CdbStackFrameContext::CdbStackFrameContext(const QSharedPointer<CdbDumperHelper> &dumper,
                                           CdbSymbolGroupContext *symbolContext) :
        m_useDumpers(dumper->isEnabled() && theDebuggerBoolSetting(UseDebuggingHelpers)),
        m_dumper(dumper),
        m_symbolContext(symbolContext)
{
}

bool CdbStackFrameContext::assignValue(const QString &iname, const QString &value,
                                       QString *newValue /* = 0 */, QString *errorMessage)
{
    return m_symbolContext->assignValue(iname, value, newValue, errorMessage);
}

bool CdbStackFrameContext::populateModelInitially(WatchHandler *wh, QString *errorMessage)
{
    if (debug)
        qDebug() << "populateModelInitially";
    const bool rc = m_useDumpers ?
        CdbSymbolGroupContext::populateModelInitially(m_symbolContext,
                                                      wh->expandedINames(),
                                                      WatchHandlerSorterInserter(wh, m_dumper),                                                      
                                                      errorMessage) :
        CdbSymbolGroupContext::populateModelInitially(m_symbolContext,
                                                      wh->expandedINames(),
                                                      WatchHandlerModelInserter(wh),
                                                      errorMessage);
    return rc;
}

bool CdbStackFrameContext::completeModel(const QList<WatchData> &incompleteLocals,
                                         WatchHandler *wh,
                                         QString *errorMessage)
{
    if (debug) {
        QDebug nsp = qDebug().nospace();
        nsp << ">completeModel ";
        foreach (const WatchData &wd, incompleteLocals)
            nsp << wd.iname << ' ';
        nsp << '\n';
    }

    if (!m_useDumpers) {
        return CdbSymbolGroupContext::completeModel(m_symbolContext, incompleteLocals,
                                                    WatchHandlerModelInserter(wh),
                                                    errorMessage);
    }

    // Expand dumper items
    int handledDumpers = 0;
    foreach (const WatchData &cwd, incompleteLocals) {
        if (cwd.source == OwnerDumper) { // You already know that.
            WatchData wd = cwd;
            wd.setAllUnneeded();
            wh->insertData(wd);
            handledDumpers++;
        }
    }
    if (handledDumpers == incompleteLocals.size())
        return true;
    // Expand symbol group items
    return CdbSymbolGroupContext::completeModel(m_symbolContext, incompleteLocals,
                                                WatchHandlerSorterInserter(wh, m_dumper),
                                                errorMessage);
}

CdbStackFrameContext::~CdbStackFrameContext()
{
    delete m_symbolContext;
}

bool CdbStackFrameContext::editorToolTip(const QString &iname,
                                         QString *value,
                                         QString *errorMessage)
{
    value->clear();
    unsigned long index;
    if (!m_symbolContext->lookupPrefix(iname, &index)) {
        *errorMessage = QString::fromLatin1("%1 not found.").arg(iname);
        return false;
    }
    const WatchData wd = m_symbolContext->symbolAt(index);
    // Check dumpers. Should actually be just one item.
    if (m_useDumpers && m_dumper->state() != CdbDumperHelper::Disabled) {
        QList<WatchData> result;
        if (CdbDumperHelper::DumpOk == m_dumper->dumpType(wd, false, OwnerDumper, &result, errorMessage))  {
            foreach (const WatchData &dwd, result) {
                if (!value->isEmpty())
                    value->append(QLatin1Char('\n'));
                value->append(dwd.toToolTip());
            }
            return true;
        } // Dumped ok
    }     // has Dumpers
    *value = wd.toToolTip();
    return true;
}

} // namespace Internal
} // namespace Debugger
