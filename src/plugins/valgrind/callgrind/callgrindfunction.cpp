/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "callgrindfunction.h"

#include "callgrindfunctioncall.h"
#include "callgrindcostitem.h"
#include "callgrindparsedata.h"
#include "callgrindfunction_p.h"

#include <utils/qtcassert.h>

#include <QString>
#include <QStringList>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>

namespace Valgrind {
namespace Callgrind {

//BEGIN Function::Private

Function::Private::Private(const ParseData *data)
    : m_data(data)
    , m_fileId(-1)
    , m_objectId(-1)
    , m_nameId(-1)
    , m_selfCost(data->events().size(), 0)
    , m_inclusiveCost(data->events().size(), 0)
    , m_called(0)
{
}

Function::Private::~Private()
{
    // we don't own m_callers
    // we own the costitem which in turn owns the callees,
    // so only delete the former
    qDeleteAll(m_costItems);

    qDeleteAll(m_outgoingCalls);
}

void Function::Private::accumulateCost(QVector<quint64> &base, const QVector<quint64> &add)
{
    if (base.isEmpty()) {
        base = add;
    } else {
        ///TODO: see whether .data() is noticably faster (less detaching)
        int i = 0;
        foreach (quint64 cost, add)
            base[i++] += cost;
    }
}

FunctionCall *Function::Private::accumulateCall(const FunctionCall *call, CallType type)
{
    const Function *key = (type == Incoming) ? call->caller() : call->callee();
    QHash<const Function *, FunctionCall *> &callMap = (type == Incoming) ? m_incomingCallMap : m_outgoingCallMap;

    FunctionCall *accumulatedCall = callMap.value(key, 0);
    if (!accumulatedCall) {
        accumulatedCall = new FunctionCall;
        if (type == Incoming)
            m_incomingCalls << accumulatedCall;
        else
            m_outgoingCalls << accumulatedCall;

        accumulatedCall->setCallee(call->callee());
        accumulatedCall->setCaller(call->caller());
        ///TODO: could the destinations differ from call to call? they should not, or?
        accumulatedCall->setDestinations(call->destinations());
        callMap.insert(key, accumulatedCall);

        accumulatedCall->setCosts(call->costs());
    } else {
        QVector<quint64> costs = accumulatedCall->costs();
        accumulateCost(costs, call->costs());
        accumulatedCall->setCosts(costs);
    }

    accumulatedCall->setCalls(accumulatedCall->calls() + call->calls());
    return accumulatedCall;
}

//BEGIN Function
Function::Function(const ParseData *data)
    : d(new Private(data))
{
}

Function::Function(Function::Private *d)
    : d(d)
{
}

Function::~Function()
{
    delete d;
}

qint64 Function::nameId() const
{
    return d->m_nameId;
}

QString Function::name() const
{
    if (d->m_nameId != -1)
        return d->m_data->stringForFunctionCompression(d->m_nameId);
    else
        return QString();
}

void Function::setName(qint64 id)
{
    d->m_nameId = id;
}

qint64 Function::fileId() const
{
    return d->m_fileId;
}

QString Function::file() const
{
    if (d->m_fileId != -1)
        return d->m_data->stringForFileCompression(d->m_fileId);
    else
        return QString();
}

void Function::setFile(qint64 id)
{
    d->m_fileId = id;
}

qint64 Function::objectId() const
{
    return d->m_objectId;
}

QString Function::object() const
{
    if (d->m_objectId != -1)
        return d->m_data->stringForObjectCompression(d->m_objectId);
    else
        return QString();
}

void Function::setObject(qint64 id)
{
    d->m_objectId = id;
}

QString Function::location() const
{
    QString pos;
    foreach (const CostItem *costItem, d->m_costItems) {
        if (costItem->differingFileId() != -1) {
            QTextStream stream(&pos);
            stream << '(';
            for (int i = 0, c = costItem->positions().count(); i < c; ++i) {
                ///TODO: remember what was hex formatted
                stream << costItem->position(i);
                if (i != c - 1)
                    stream << ", ";
            }
            stream << ')';
            break;
        }
    }
    QString f = file();

    if (!f.isEmpty()) {
        QFileInfo info(f);
        if (info.exists())
            f = info.canonicalFilePath();
    }

    QString o = object();
    if (o.isEmpty())
        return QString();
    if (f.isEmpty() || f == QLatin1String("???"))
        return o;
    if (pos.isEmpty())
        return QCoreApplication::translate("Valgrind::Callgrind::Function", "%1 in %2").arg(f, o);

    return QCoreApplication::translate("Valgrind::Callgrind::Function", "%1:%2 in %3").arg(f, pos, o);
}

int Function::lineNumber() const
{
    const int lineIdx = d->m_data->lineNumberPositionIndex();
    if (lineIdx == -1)
        return -1;

    foreach (const CostItem *costItem, d->m_costItems) {
        if (costItem->differingFileId() == -1)
            return costItem->position(lineIdx);
    }

    return -1;
}

quint64 Function::selfCost(int event) const
{
    return d->m_selfCost.at(event);
}

QVector< quint64 > Function::selfCosts() const
{
    return d->m_selfCost;
}

quint64 Function::inclusiveCost(int event) const
{
    return d->m_inclusiveCost.at(event) + d->m_selfCost.at(event);
}

QVector<const FunctionCall *> Function::outgoingCalls() const
{
    return d->m_outgoingCalls;
}

void Function::addOutgoingCall(const FunctionCall *call)
{
    QTC_ASSERT(call->caller() == this, return);

    d->accumulateCall(call, Private::Outgoing);
}

QVector<const FunctionCall *> Function::incomingCalls() const
{
    return d->m_incomingCalls;
}

void Function::addIncomingCall(const FunctionCall *call)
{
    QTC_ASSERT(call->callee() == this, return);
    d->m_called += call->calls();
    d->accumulateCall(call, Private::Incoming);
}

quint64 Function::called() const
{
    return d->m_called;
}

QVector<const CostItem *> Function::costItems() const
{
    return d->m_costItems;
}

void Function::addCostItem(const CostItem *item)
{
    QTC_ASSERT(!d->m_costItems.contains(item), return);

    d->m_costItems.append(item);

    // accumulate costs
    if (item->call())
        d->accumulateCost(d->m_inclusiveCost, item->costs());
    else
        d->accumulateCost(d->m_selfCost, item->costs());
}

void Function::finalize()
{
    bool recursive = false;
    foreach (const FunctionCall *call, d->m_incomingCalls) {
        if (call->caller() == this) {
            recursive = true;
            break;
        }
    }

    if (recursive) {
        // now handle recursive calls by setting the incl cost to the sum of all (external) calls
        // to this function
        // e.g.: A -> B -> B ..., C -> B -> B ...
        // cost of B = cost of call to B in A + cost of call to B in C + ...
        d->m_inclusiveCost.fill(0);
        foreach (const FunctionCall *call, d->m_incomingCalls) {
            if (call->caller() != this) {
                foreach (const CostItem *costItem, call->caller()->costItems()) {
                    if (costItem->call() && costItem->call()->callee() == this)
                        d->accumulateCost(d->m_inclusiveCost, costItem->costs());
                }
            }
        }
        // now subtract self cost (see @c inclusiveCost() implementation)
        for (int i = 0, c = d->m_inclusiveCost.size(); i < c; ++i) {
            if (d->m_inclusiveCost.at(i) < d->m_selfCost.at(i))
                d->m_inclusiveCost[i] = 0;
            else
                d->m_inclusiveCost[i] -= d->m_selfCost.at(i);
        }
    }
}

} // namespace Callgrind
} // namespace Valgrind
