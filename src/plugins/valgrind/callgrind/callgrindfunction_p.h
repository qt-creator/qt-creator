/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "callgrindfunction.h"
#include "callgrindparsedata.h"
#include "callgrindcostitem.h"
#include "callgrindfunctioncall.h"

#include <QVector>
#include <QHash>

namespace Valgrind {
namespace Callgrind {

class Function::Private
{
public:
    Private(const ParseData *data);
    virtual ~Private();

    static void accumulateCost(QVector<quint64> &base, const QVector<quint64> &add);
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

    QVector<quint64> m_selfCost;
    QVector<quint64> m_inclusiveCost;

    QVector<const CostItem *> m_costItems;
    // used to accumulate, hence values not const
    QHash<const Function *, FunctionCall *> m_outgoingCallMap;
    QHash<const Function *, FunctionCall *> m_incomingCallMap;
    // used in public api, hence const
    QVector<const FunctionCall *> m_outgoingCalls;
    QVector<const FunctionCall *> m_incomingCalls;
    quint64 m_called = 0;
};

} // namespace Callgrind
} // namespace Valgrind
