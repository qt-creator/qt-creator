// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindfunctioncycle.h"

#include "callgrindfunction_p.h"
#include "callgrindfunctioncall.h"
#include "callgrindparsedata.h"

namespace Valgrind::Callgrind {

//BEGIN FunctionCycle::Private

class FunctionCycle::Private : public Function::Private
{
public:
    Private(const ParseData *data);
    QList<const Function *> m_functions;
};

FunctionCycle::Private::Private(const ParseData *data)
    : Function::Private(data)
{
}

#define CYCLE_D static_cast<FunctionCycle::Private *>(this->d)

//BEGIN FunctionCycle

FunctionCycle::FunctionCycle(const ParseData *data)
    : Function(new Private(data))
{
}

// d should be deleted by Function::~Function()
FunctionCycle::~FunctionCycle() = default;

void FunctionCycle::setFunctions(const QList<const Function *> &functions)
{
    Private *d = CYCLE_D;

    d->m_functions = functions;

    d->m_incomingCallMap.clear();
    d->m_outgoingCallMap.clear();
    d->m_called = 0;
    d->m_selfCost.fill(0, d->m_data->events().size());
    d->m_inclusiveCost.fill(0, d->m_data->events().size());

    for (const Function *func : functions) {
        // just add up self cost
        Private::accumulateCost(d->m_selfCost, func->selfCosts());
        // add outgoing calls to functions that are not part of the cycle
        const QList<const FunctionCall *> calls = func->outgoingCalls();
        for (const FunctionCall *call : calls) {
            if (!functions.contains(call->callee()))
                d->accumulateCall(call, Function::Private::Outgoing);
        }
        // add incoming calls from functions that are not part of the cycle
        const QList<const FunctionCall *> inCalls = func->incomingCalls();
        for (const FunctionCall *call : inCalls) {
            if (!functions.contains(call->caller())) {
                d->accumulateCall(call, Function::Private::Incoming);
                d->m_called += call->calls();
                Private::accumulateCost(d->m_inclusiveCost, call->costs());
            }
        }
    }
    // now subtract self from incl. cost (see implementation of inclusiveCost())
    // now subtract self cost (see @c inclusiveCost() implementation)
    for (int i = 0, c = d->m_inclusiveCost.size(); i < c; ++i) {
        if (d->m_inclusiveCost.at(i) < d->m_selfCost.at(i))
            d->m_inclusiveCost[i] = 0;
        else
            d->m_inclusiveCost[i] -= d->m_selfCost.at(i);
    }
}

QList<const Function *> FunctionCycle::functions() const
{
    return CYCLE_D->m_functions;
}

} // namespace Valgrind::Callgrind
