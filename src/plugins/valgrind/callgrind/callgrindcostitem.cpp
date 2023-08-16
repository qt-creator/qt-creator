// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindcostitem.h"

#include "callgrindparsedata.h"
#include "callgrindfunctioncall.h"

#include <QStringList>

namespace Valgrind::Callgrind {

class CostItem::Private
{
public:
    Private(ParseData *data);
    ~Private();

    QList<quint64> m_positions;
    QList<quint64> m_events;
    const FunctionCall *m_call = nullptr;

    const ParseData *m_data = nullptr;
    qint64 m_differingFileId = -1;
};

CostItem::Private::Private(ParseData *data)
    : m_positions(data->positions().size(), 0)
    , m_events(data->events().size(), 0)
    , m_data(data)
{
}

CostItem::Private::~Private()
{
    delete m_call;
}


//BEGIN CostItem
CostItem::CostItem(ParseData *data)
: d(new Private(data))
{
}

CostItem::~CostItem()
{
    delete d;
}

quint64 CostItem::position(int posIdx) const
{
    return d->m_positions.at(posIdx);
}

void CostItem::setPosition(int posIdx, quint64 position)
{
    d->m_positions[posIdx] = position;
}

QList<quint64> CostItem::positions() const
{
    return d->m_positions;
}

quint64 CostItem::cost(int event) const
{
    return d->m_events.at(event);
}

void CostItem::setCost(int event, quint64 cost)
{
    d->m_events[event] = cost;
}

QList<quint64> CostItem::costs() const
{
    return d->m_events;
}

const FunctionCall *CostItem::call() const
{
    return d->m_call;
}

void CostItem::setCall(const FunctionCall *call)
{
    d->m_call = call;
}

QString CostItem::differingFile() const
{
    if (d->m_differingFileId != -1)
        return d->m_data->stringForFileCompression(d->m_differingFileId);
    else
        return QString();
}

qint64 CostItem::differingFileId() const
{
    return d->m_differingFileId;
}

void CostItem::setDifferingFile(qint64 fileId)
{
    d->m_differingFileId = fileId;
}

} // Valgrind::Callgrind
