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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "cdbstackframecontext.h"
#include "cdbdebugengine_p.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdumperhelper.h"
#include "debuggeractions.h"
#include "watchhandler.h"

#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

enum { OwnerNewItem, OwnerSymbolGroup, OwnerDumper };

typedef QSharedPointer<CdbDumperHelper> SharedPointerCdbDumperHelper;
typedef QList<WatchData> WatchDataList;

// Predicates for parametrizing the symbol group
inline bool truePredicate(const WatchData & /* whatever */) { return true; }
inline bool falsePredicate(const WatchData & /* whatever */) { return false; }
inline bool isDumperPredicate(const WatchData &wd)
{ return wd.source == OwnerDumper; }

// Match an item that is expanded in the watchhandler.
class WatchHandlerExpandedPredicate {
public:
    explicit inline WatchHandlerExpandedPredicate(const WatchHandler *wh) : m_wh(wh) {}
    inline bool operator()(const WatchData &wd) { return m_wh->isExpandedIName(wd.iname); }
private:
    const WatchHandler *m_wh;
};

// Match an item by iname
class MatchINamePredicate {
public:
    explicit inline MatchINamePredicate(const QString &iname) : m_iname(iname) {}
    inline bool operator()(const WatchData &wd) { return wd.iname == m_iname; }
private:
    const QString &m_iname;
};

// Put a sequence of  WatchData into the model for the non-dumper case
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

// Put a sequence of  WatchData into the model for the dumper case.
// Sorts apart a sequence of  WatchData using the Dumpers.
// Puts the stuff for which there is no dumper in the model
// as is and sets ownership to symbol group. The rest goes
// to the dumpers. Additionally, checks for items pointing to a
// dumpeable type and inserts a fake dereferenced item and value.
class WatchHandleDumperInserter {
public:
    explicit WatchHandleDumperInserter(WatchHandler *wh,
                                        const SharedPointerCdbDumperHelper &dumper);

    inline WatchHandleDumperInserter & operator*() { return *this; }
    inline WatchHandleDumperInserter &operator=(WatchData &wd);
    inline WatchHandleDumperInserter &operator++() { return *this; }

private:
    bool expandPointerToDumpable(const WatchData &wd, QString *errorMessage);

    const QRegExp m_hexNullPattern;
    WatchHandler *m_wh;
    const SharedPointerCdbDumperHelper m_dumper;
    QList<WatchData> m_dumperResult;
};

WatchHandleDumperInserter::WatchHandleDumperInserter(WatchHandler *wh,
                                                       const SharedPointerCdbDumperHelper &dumper) :
    m_hexNullPattern(QLatin1String("0x0+")),
    m_wh(wh),
    m_dumper(dumper)
{
    Q_ASSERT(m_hexNullPattern.isValid());
}

// Is this a non-null pointer to a dumpeable item with a value
// "0x4343 class QString *" ? - Insert a fake '*' dereferenced item
// and run dumpers on it. If that succeeds, insert the fake items owned by dumpers,
// which will trigger the ignore predicate.
// Note that the symbol context does not create '*' dereferenced items for
// classes (see note in its header documentation).
bool WatchHandleDumperInserter::expandPointerToDumpable(const WatchData &wd, QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << ">expandPointerToDumpable" << wd.iname;

    bool handled = false;
    do {
        if (!isPointerType(wd.type))
            break;
        const int classPos = wd.value.indexOf(" class ");
        if (classPos == -1)
            break;
        const QString hexAddrS = wd.value.mid(0, classPos);
        if (m_hexNullPattern.exactMatch(hexAddrS))
            break;
        const QString type = stripPointerType(wd.value.mid(classPos + 7));
        WatchData derefedWd;
        derefedWd.setType(type);
        derefedWd.setAddress(hexAddrS);
        derefedWd.name = QString(QLatin1Char('*'));
        derefedWd.iname = wd.iname + QLatin1String(".*");
        derefedWd.source = OwnerDumper;
        const CdbDumperHelper::DumpResult dr = m_dumper->dumpType(derefedWd, true, OwnerDumper, &m_dumperResult, errorMessage);
        if (dr != CdbDumperHelper::DumpOk)
            break;
        // Insert the pointer item with 1 additional child + its dumper results
        // Note: formal arguments might already be expanded in the symbol group.
        WatchData ptrWd = wd;
        ptrWd.source = OwnerDumper;
        ptrWd.setHasChildren(true);
        ptrWd.setChildrenUnneeded();
        m_wh->insertData(ptrWd);
        foreach(const WatchData &dwd, m_dumperResult)
            m_wh->insertData(dwd);
        handled = true;
    } while (false);
    if (debugCDBWatchHandling)
        qDebug() << "<expandPointerToDumpable returns " << handled << *errorMessage;
    return handled;
}

WatchHandleDumperInserter &WatchHandleDumperInserter::operator=(WatchData &wd)
{
    if (debugCDBWatchHandling)
        qDebug() << "WatchHandleDumperInserter::operator=" << wd.toString();
    // Check pointer to dumpeable, dumpeable, insert accordingly.
    QString errorMessage;
    if (expandPointerToDumpable(wd, &errorMessage)) {
        // Nasty side effect: Modify owner for the ignore predicate
        wd.source = OwnerDumper;
        return *this;
    }
    switch (m_dumper->dumpType(wd, true, OwnerDumper, &m_dumperResult, &errorMessage)) {
    case CdbDumperHelper::DumpOk:
        if (debugCDBWatchHandling)
            qDebug() << "dumper triggered";
        // Dumpers omit types for complicated templates
        if (!m_dumperResult.isEmpty() && m_dumperResult.front().type.isEmpty()
            && m_dumperResult.front().iname == wd.iname)
            m_dumperResult.front().setType(wd.type);
        // Discard the original item and insert the dumper results
        foreach(const WatchData &dwd, m_dumperResult)
            m_wh->insertData(dwd);
        // Nasty side effect: Modify owner for the ignore predicate
        wd.source = OwnerDumper;
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
    if (debugCDBWatchHandling)
        qDebug() << "populateModelInitially dumpers=" << m_useDumpers;
    // Recurse down items that are initially expanded in the view, stop processing for
    // dumper items.
    const bool rc = m_useDumpers ?
        CdbSymbolGroupContext::populateModelInitially(m_symbolContext,
                                                      WatchHandleDumperInserter(wh, m_dumper),
                                                      WatchHandlerExpandedPredicate(wh),
                                                      isDumperPredicate,
                                                      errorMessage) :
        CdbSymbolGroupContext::populateModelInitially(m_symbolContext,
                                                      WatchHandlerModelInserter(wh),
                                                      WatchHandlerExpandedPredicate(wh),
                                                      falsePredicate,
                                                      errorMessage);
    return rc;
}

bool CdbStackFrameContext::completeData(const WatchData &incompleteLocal,
                                        WatchHandler *wh,
                                        QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << ">completeData src=" << incompleteLocal.source << incompleteLocal.toString();

    // Expand symbol group items, recurse one level from desired item
    if (!m_useDumpers) {
        return CdbSymbolGroupContext::completeData(m_symbolContext, incompleteLocal,
                                                   WatchHandlerModelInserter(wh),
                                                   MatchINamePredicate(incompleteLocal.iname),
                                                   falsePredicate,
                                                   errorMessage);
    }

    // Expand artifical dumper items
    if (incompleteLocal.source == OwnerDumper) {
        QList<WatchData> dumperResult;
        const CdbDumperHelper::DumpResult dr = m_dumper->dumpType(incompleteLocal, true, OwnerDumper, &dumperResult, errorMessage);
        if (dr == CdbDumperHelper::DumpOk) {
            foreach(const WatchData &dwd, dumperResult)
                wh->insertData(dwd);
        } else {
            const QString msg = QString::fromLatin1("Unable to further expand dumper watch data: %1 (%2): %3/%4").arg(incompleteLocal.name, incompleteLocal.type).arg(int(dr)).arg(*errorMessage);
            qWarning("%s", qPrintable(msg));
            WatchData wd = incompleteLocal;
            wd.setAllUnneeded();
            wh->insertData(wd);
        }
        return true;
    }

    // Expand symbol group items, recurse one level from desired item
    return CdbSymbolGroupContext::completeData(m_symbolContext, incompleteLocal,
                                               WatchHandleDumperInserter(wh, m_dumper),
                                               MatchINamePredicate(incompleteLocal.iname),
                                               isDumperPredicate,
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
