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

#include "callgrindfunctioncall.h"

#include "callgrindfunction.h"

#include <utils/qtcassert.h>

#include <QVector>

namespace Valgrind {
namespace Callgrind {

//BEGIN FunctionCall::Private
class FunctionCall::Private
{
public:
    explicit Private();

    const Function *m_callee;
    const Function *m_caller;
    quint64 m_calls;
    quint64 m_totalInclusiveCost;
    QVector<quint64> m_destinations;
    QVector<quint64> m_costs;
};

FunctionCall::Private::Private()
    : m_callee(0)
    , m_caller(0)
    , m_calls(0)
    , m_totalInclusiveCost(0)
{
}

//BEGIN FunctionCall

FunctionCall::FunctionCall()
    : d(new Private)
{
}

FunctionCall::~FunctionCall()
{
    delete d;
}

const Function *FunctionCall::callee() const
{
    return d->m_callee;
}

void FunctionCall::setCallee(const Function *function)
{
    d->m_callee = function;
}

const Function *FunctionCall::caller() const
{
    return d->m_caller;
}

void FunctionCall::setCaller(const Function *function)
{
    d->m_caller = function;
}

quint64 FunctionCall::calls() const
{
    return d->m_calls;
}

void FunctionCall::setCalls(quint64 calls)
{
    d->m_calls = calls;
}

quint64 FunctionCall::destination(int posIdx) const
{
    return d->m_destinations.at(posIdx);
}

QVector<quint64> FunctionCall::destinations() const
{
    return d->m_destinations;
}

void FunctionCall::setDestinations(const QVector<quint64> &destinations)
{
    d->m_destinations = destinations;
}

quint64 FunctionCall::cost(int event) const
{
    QTC_ASSERT(event >= 0 && event < d->m_costs.size(), return 0);
    return d->m_costs.at(event);
}

QVector<quint64> FunctionCall::costs() const
{
    return d->m_costs;
}

void FunctionCall::setCosts(const QVector<quint64> &costs)
{
    d->m_costs = costs;
}

} // namespace Callgrind
} // namespace Valgrind
