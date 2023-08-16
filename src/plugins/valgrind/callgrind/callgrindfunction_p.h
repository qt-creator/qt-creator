// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "callgrindcostitem.h"
#include "callgrindfunction.h"
#include "callgrindfunctioncall.h"
#include "callgrindparsedata.h"

#include <QHash>
#include <QList>

namespace Valgrind::Callgrind {

class Function::Private
{
public:
    Private(const ParseData *data);
    virtual ~Private();

    static void accumulateCost(QList<quint64> &base, const QList<quint64> &add);
    enum CallType {
        Incoming,
        Outgoing
    };
    ///@return accumulated call
    FunctionCall *accumulateCall(const FunctionCall *call, CallType type);

    const ParseData *m_data;
    qint64 m_fileId = -1;
    qint64 m_objectId = -1;
    qint64 m_nameId = -1;

    QList<quint64> m_selfCost;
    QList<quint64> m_inclusiveCost;

    QList<const CostItem *> m_costItems;
    // used to accumulate, hence values not const
    QHash<const Function *, FunctionCall *> m_outgoingCallMap;
    QHash<const Function *, FunctionCall *> m_incomingCallMap;
    // used in public api, hence const
    QList<const FunctionCall *> m_outgoingCalls;
    QList<const FunctionCall *> m_incomingCalls;
    quint64 m_called = 0;
};

} // namespace Valgrind::Callgrind
