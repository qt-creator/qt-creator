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

#include "cdbwatchmodels.h"
#include "cdbdumperhelper.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdebugengine.h"
#include "watchutils.h"
#include "debuggeractions.h"

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QCoreApplication>
#include <QtCore/QRegExp>

enum { debugCDBWatchHandling = 0 };

namespace Debugger {
namespace Internal {

enum { LocalsOwnerSymbolGroup, LocalsOwnerDumper };

static inline QString msgNoStackFrame()
{
    return QCoreApplication::translate("CdbWatchModels", "No active stack frame present.");
}

static inline QString msgUnknown()
{
    return QCoreApplication::translate("CdbWatchModels", "<Unknown>");
}

// Helper to add a sequence of WatchData from the symbol group context to an item
// without using dumpers.

class SymbolGroupInserter
{
public:
    explicit SymbolGroupInserter(WatchItem *parent) : m_parent(parent) {}

    inline SymbolGroupInserter& operator*() { return *this; }
    inline SymbolGroupInserter&operator=(const WatchData &wd) {
        WatchItem *item = new WatchItem(wd);
        item->source = LocalsOwnerSymbolGroup;
        m_parent->addChild(item);
        return *this;
    }
    inline SymbolGroupInserter&operator++() { return *this; }

private:
    WatchItem *m_parent;
};

// fixDumperResult: When querying an item, the queried item is sometimes returned in
// incomplete form. Take over values from source, set items with missing addresses to
// "complete".
static inline void fixDumperResult(const WatchData &source,
                                   QList<WatchData> *result)
{
    if (debugCDBWatchHandling > 1) {
        qDebug() << "fixDumperResult for : " << source.toString();
        foreach (const WatchData &wd, *result)
            qDebug() << "   " << wd.toString();
    }
    const int size = result->size();
    if (!size)
        return;
    WatchData &returned = result->front();
    if (returned.iname != source.iname)
        return;
    if (returned.type.isEmpty())
        returned.setType(source.type);
    if (returned.isValueNeeded()) {
        if (source.isValueKnown()) {
            returned.setValue(source.value);
        } else {
            // Should not happen
            returned.setValue(msgUnknown());
            qWarning("%s: No value for %s\n", Q_FUNC_INFO, qPrintable(returned.toString()));
        }
    }
    if (size == 1)
        return;
    // Fix the children: If the address is missing, we cannot query any further.
    const QList<WatchData>::iterator wend = result->end();
    QList<WatchData>::iterator it = result->begin();
    for (++it; it != wend; ++it) {
        WatchData &wd = *it;
        if (wd.addr.isEmpty() && wd.isSomethingNeeded()) {
            wd.setAllUnneeded();
        }
    }
}

// Dump an item. If *ptrToDumpedItem == 0, allocate a new item and set it.
// If it is non-null, the item pointed to will receive the results
// ("complete" functionality).
static CdbDumperHelper::DumpResult
        dumpItem(const QSharedPointer<CdbDumperHelper> dumper,
                 const WatchData &wd,
                 WatchItem **ptrToDumpedItem,
                 int dumperOwnerValue, QString *errorMessage)
{
    QList<WatchData> dumperResult;
    WatchItem *dumpedItem = *ptrToDumpedItem;
    const CdbDumperHelper::DumpResult rc = dumper->dumpType(wd, true, dumperOwnerValue, &dumperResult, errorMessage);
    if (debugCDBWatchHandling > 1)
        qDebug() << "dumper for " << wd.type << " returns " << rc;

    switch (rc) {
    case CdbDumperHelper::DumpError:
        return rc;
    case CdbDumperHelper::DumpNotHandled:
        errorMessage->clear();
        return rc;
    case CdbDumperHelper::DumpOk:
        break;
    }
    // Dumpers omit types for complicated templates
    fixDumperResult(wd, &dumperResult);
    // Discard the original item and insert the dumper results
    if (dumpedItem) {
        dumpedItem->setData(dumperResult.front());
    } else {
        dumpedItem = new WatchItem(dumperResult.front());
        *ptrToDumpedItem = dumpedItem;
    }
    dumperResult.pop_front();
    foreach(const WatchData &dwd, dumperResult)
        dumpedItem->addChild(new WatchItem(dwd));
    return rc;
}

// Is this a non-null pointer to a dumpeable item with a value
// "0x4343 class QString *" ? - Insert a fake '*' dereferenced item
// and run dumpers on it. If that succeeds, insert the fake items owned by dumpers,
// Note that the symbol context does not create '*' dereferenced items for
// classes (see note in its header documentation).
static bool expandPointerToDumpable(const QSharedPointer<CdbDumperHelper> dumper,
                                    const WatchData &wd,
                                    WatchItem *parent,
                                    QString *errorMessage)
{


    if (debugCDBWatchHandling > 1)
        qDebug() << ">expandPointerToDumpable" << wd.iname;

    WatchItem *derefedWdItem = 0;
    WatchItem *ptrWd = 0;
    bool handled = false;
    do {
        if (!isPointerType(wd.type))
            break;
        const int classPos = wd.value.indexOf(" class ");
        if (classPos == -1)
            break;
        const QString hexAddrS = wd.value.mid(0, classPos);
        static const QRegExp hexNullPattern(QLatin1String("0x0+"));
        Q_ASSERT(hexNullPattern.isValid());
        if (hexNullPattern.exactMatch(hexAddrS))
            break;
        const QString type = stripPointerType(wd.value.mid(classPos + 7));
        WatchData derefedWd;
        derefedWd.setType(type);
        derefedWd.setAddress(hexAddrS);
        derefedWd.name = QString(QLatin1Char('*'));
        derefedWd.iname = wd.iname + QLatin1String(".*");
        derefedWd.source = LocalsOwnerDumper;
        QList<WatchData> dumperResult;
        const CdbDumperHelper::DumpResult dr = dumpItem(dumper, derefedWd, &derefedWdItem, LocalsOwnerDumper, errorMessage);
        if (dr != CdbDumperHelper::DumpOk)
            break;
        // Insert the pointer item with 1 additional child + its dumper results
        // Note: formal arguments might already be expanded in the symbol group.
        ptrWd = new WatchItem(wd);
        ptrWd->source = LocalsOwnerDumper;
        ptrWd->setHasChildren(true);
        ptrWd->setChildrenUnneeded();
        ptrWd->addChild(derefedWdItem);
        parent->addChild(ptrWd);
        handled = true;
    } while (false);
    if (!handled) {
        delete derefedWdItem;
        delete ptrWd;
    }
    if (debugCDBWatchHandling > 1)
        qDebug() << "<expandPointerToDumpable returns " << handled << *errorMessage;
    return handled;
}

// Main routine for inserting an item from the symbol group using the dumpers
// where applicable.
static inline bool insertDumpedItem(const QSharedPointer<CdbDumperHelper> &dumper,
                                    const WatchData &wd,
                                    WatchItem *parent,
                                    QString *errorMessage)
{
    if (debugCDBWatchHandling > 1)
        qDebug() << "insertItem=" << wd.toString();
    // Check pointer to dumpeable, dumpeable, insert accordingly.
    if (expandPointerToDumpable(dumper, wd, parent, errorMessage))
        return true;
    WatchItem *dumpedItem = 0;
    // Add item owned by Dumper or symbol group on failure.
    const CdbDumperHelper::DumpResult dr = dumpItem(dumper, wd, &dumpedItem, LocalsOwnerDumper, errorMessage);
    switch (dr) {
        case CdbDumperHelper::DumpOk:
        dumpedItem->parent = parent;
        parent->children.push_back(dumpedItem);
        break;
    case CdbDumperHelper::DumpNotHandled:
        case CdbDumperHelper::DumpError: {
        WatchItem *symbolItem = new WatchItem(wd);
        symbolItem->source = LocalsOwnerSymbolGroup;
        parent->addChild(symbolItem);
    }
        break;
    }
    return true;
}

// Helper to add a sequence of  WatchData from the symbol group context to an item.
// Checks if the item is dumpable in some way; if so, dump it and use that instead of
// symbol group.
class SymbolGroupDumperInserter
{
public:
    explicit SymbolGroupDumperInserter(const QSharedPointer<CdbDumperHelper> &dumper,
                                       WatchItem *parent,
                                       QString *errorMessage);

    inline SymbolGroupDumperInserter& operator*() { return *this; }
    inline SymbolGroupDumperInserter&operator=(const WatchData &wd) {
        insertDumpedItem(m_dumper, wd, m_parent, m_errorMessage);
        return *this;
    }
    inline SymbolGroupDumperInserter&operator++() { return *this; }

private:
    const QSharedPointer<CdbDumperHelper> m_dumper;
    WatchItem *m_parent;
    QString *m_errorMessage;
};

SymbolGroupDumperInserter::SymbolGroupDumperInserter(const QSharedPointer<CdbDumperHelper> &dumper,
                                              WatchItem *parent,
                                              QString *errorMessage) :
    m_dumper(dumper),
    m_parent(parent),
    m_errorMessage(errorMessage)
{
}

// -------------- CdbLocalsModel

CdbLocalsModel::CdbLocalsModel(const QSharedPointer<CdbDumperHelper> &dh,
                               WatchHandler *handler, WatchType type, QObject *parent) :
    AbstractSyncWatchModel(handler, type, parent),
    m_dumperHelper(dh),
    m_symbolGroupContext(0),
    m_useDumpers(false)
{
}

CdbLocalsModel::~CdbLocalsModel()
{
}

bool CdbLocalsModel::fetchChildren(WatchItem *wd, QString *errorMessage)
{
    if (!m_symbolGroupContext) {
        *errorMessage = msgNoStackFrame();
        return false;
    }
    if (debugCDBWatchHandling)
        qDebug() << "fetchChildren" << wd->iname;

    // Check the owner and call it to expand the item.
    switch (wd->source) {
    case LocalsOwnerSymbolGroup:
        if (m_useDumpers && m_dumperHelper->isEnabled()) {
            SymbolGroupDumperInserter inserter(m_dumperHelper, wd, errorMessage);
            return m_symbolGroupContext->getChildSymbols(wd->iname, inserter, errorMessage);
        } else {
            return m_symbolGroupContext->getChildSymbols(wd->iname, SymbolGroupInserter(wd), errorMessage);
        }
        break;
    case LocalsOwnerDumper:
        if (dumpItem(m_dumperHelper, *wd, &wd, LocalsOwnerDumper, errorMessage) == CdbDumperHelper::DumpOk)
            return true;
        if (wd->isValueNeeded())
            wd->setValue(msgUnknown());
        qWarning("%s: No value for %s\n", Q_FUNC_INFO, qPrintable(wd->toString()));
        return false;
    }
    return false;
}

bool CdbLocalsModel::complete(WatchItem *wd, QString *errorMessage)
{
    if (!m_symbolGroupContext) {
        *errorMessage = msgNoStackFrame();
        return false;
    }
    if (debugCDBWatchHandling)
        qDebug() << "complete" << wd->iname;
    // Might as well fetch children when completing a dumped item.
    return fetchChildren(wd, errorMessage);
}

void CdbLocalsModel::setSymbolGroupContext(CdbSymbolGroupContext *s)
{
    if (s == m_symbolGroupContext)
        return;
    if (debugCDBWatchHandling)
        qDebug() << ">setSymbolGroupContext" << s;
    m_symbolGroupContext = s;
    reinitialize();
    if (!m_symbolGroupContext)
        return;
    // Populate first row
    WatchItem *item = dummyRoot();
    QString errorMessage;
    do {
        if (m_useDumpers && m_dumperHelper->isEnabled()) {
            SymbolGroupDumperInserter inserter(m_dumperHelper, item, &errorMessage);
            if (!m_symbolGroupContext->getChildSymbols(m_symbolGroupContext->prefix(), inserter, &errorMessage)) {
                break;
            }
        } else {
            if (!m_symbolGroupContext->getChildSymbols(m_symbolGroupContext->prefix(), SymbolGroupInserter(item), &errorMessage))
                break;
        }
        if (item->children.empty())
            break;
        reset();
    } while (false);
    if (!errorMessage.isEmpty())
        emit error(errorMessage);
    if (debugCDBWatchHandling)
        qDebug() << "<setSymbolGroupContext" << item->children.size() << errorMessage;
    if (debugCDBWatchHandling > 1)
        qDebug() << '\n' << *this;
}

// ---- CdbWatchModel

enum { WatchOwnerNewItem, WatchOwnerExpression, WatchOwnerDumper };

CdbWatchModel::CdbWatchModel(CdbDebugEngine *engine,
                             const QSharedPointer<CdbDumperHelper> &dh,
                             WatchHandler *handler,
                             WatchType type, QObject *parent) :
   AbstractSyncWatchModel(handler, type, parent),
   m_dumperHelper(dh),
   m_engine(engine)
{
}

CdbWatchModel::~CdbWatchModel()
{
}

bool CdbWatchModel::evaluateWatchExpression(WatchData *wd, QString *errorMessage)
{
    QString value;
    QString type;

    const bool rc = m_engine->evaluateExpression(wd->exp, &value, &type, errorMessage);
    if (!rc) {
        wd->setValue(msgUnknown());
        return false;
    }
    wd->setValue(value);
    wd->setType(type);
    return true;
}

bool CdbWatchModel::fetchChildren(WatchItem *wd, QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << "Watch:fetchChildren" << wd->iname << wd->source;
    // We need to be halted.
    if (!m_engine->isDebuggeeHalted()) {
        *errorMessage = QCoreApplication::translate("CdbWatchModels", "Can evaluates watches only in halted state.");
        wd->setValue(msgUnknown());
        return false;
    }
    // New item with address -> dumper.
    if (wd->source == WatchOwnerNewItem)
        wd->source = wd->addr.isEmpty() ? WatchOwnerExpression : WatchOwnerDumper;
    // Expressions
    if (wd->source == WatchOwnerExpression)
        return evaluateWatchExpression(wd, errorMessage);
    // Variables by address
    if (!m_dumperHelper->isEnabled() || !theDebuggerBoolSetting(UseDebuggingHelpers)) {
        *errorMessage = QCoreApplication::translate("CdbWatchModels", "Cannot evaluate '%1' due to dumpers being disabled.").arg(wd->name);
        return false;
    }
    return dumpItem(m_dumperHelper, *wd, &wd, WatchOwnerDumper, errorMessage) == CdbDumperHelper::DumpOk;
}

bool CdbWatchModel::complete(WatchItem *wd, QString *errorMessage)
{
    return fetchChildren(wd, errorMessage);
}

void CdbWatchModel::addWatcher(const WatchData &wd)
{
    WatchItem *root = dummyRoot();
    if (!root)
        return;
    const QModelIndex rootIndex = watchIndex(root);
    beginInsertRows(rootIndex, root->children.size(), root->children.size());
    root->addChild(new WatchItem(wd));
    endInsertRows();
}

void CdbWatchModel::refresh()
{
    // Refresh data for a new break
    WatchItem *root = dummyRoot();
    if (!root)
        return;
    const int childCount = root->children.size();
    if (!childCount)
        return;
    // reset flags, if there children, trigger a reset
    bool resetRequired = false;
    for (int i = 0; i < childCount; i++) {
        WatchItem *topLevel = root->children.at(i);
        topLevel->setAllNeeded();
        if (!topLevel->children.empty()) {
            topLevel->removeChildren();
            resetRequired = true;
        }
    }
    // notify model
    if (resetRequired) {
        reset();
    } else {
        const QModelIndex topLeft = watchIndex(root->children.front());
        const QModelIndex bottomLeft = root->children.size() == 1 ? topLeft : watchIndex(root->children.back());
        emit dataChanged(topLeft.sibling(0, 1), bottomLeft.sibling(0, columnCount() -1));
    }
}

} // namespace Internal
} // namespace Debugger
