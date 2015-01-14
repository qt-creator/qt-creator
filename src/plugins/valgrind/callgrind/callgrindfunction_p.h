/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBVALGRIND_CALLGRINDFUNCTION_P_H
#define LIBVALGRIND_CALLGRINDFUNCTION_P_H

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
    ~Private();

    static void accumulateCost(QVector<quint64> &base, const QVector<quint64> &add);
    enum CallType {
        Incoming,
        Outgoing
    };
    ///@return accumulated call
    FunctionCall *accumulateCall(const FunctionCall *call, CallType type);

    const ParseData *m_data;
    qint64 m_fileId;
    qint64 m_objectId;
    qint64 m_nameId;

    QVector<quint64> m_selfCost;
    QVector<quint64> m_inclusiveCost;

    QVector<const CostItem *> m_costItems;
    // used to accumulate, hence values not const
    QHash<const Function *, FunctionCall *> m_outgoingCallMap;
    QHash<const Function *, FunctionCall *> m_incomingCallMap;
    // used in public api, hence const
    QVector<const FunctionCall *> m_outgoingCalls;
    QVector<const FunctionCall *> m_incomingCalls;
    quint64 m_called;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // LIBVALGRIND_CALLGRINDFUNCTION_P_H
