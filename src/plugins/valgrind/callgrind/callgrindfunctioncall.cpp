// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindfunctioncall.h"

#include "callgrindfunction.h"

#include <utils/qtcassert.h>

#include <QList>

namespace Valgrind::Callgrind {

//BEGIN FunctionCall::Private
class FunctionCall::Private
{
public:
    const Function *m_callee = nullptr;
    const Function *m_caller = nullptr;
    quint64 m_calls = 0;
    quint64 m_totalInclusiveCost = 0;
    QList<quint64> m_destinations;
    QList<quint64> m_costs;
};

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

QList<quint64> FunctionCall::destinations() const
{
    return d->m_destinations;
}

void FunctionCall::setDestinations(const QList<quint64> &destinations)
{
    d->m_destinations = destinations;
}

quint64 FunctionCall::cost(int event) const
{
    QTC_ASSERT(event >= 0 && event < d->m_costs.size(), return 0);
    return d->m_costs.at(event);
}

QList<quint64> FunctionCall::costs() const
{
    return d->m_costs;
}

void FunctionCall::setCosts(const QList<quint64> &costs)
{
    d->m_costs = costs;
}

} // namespace Valgrind::Callgrind
