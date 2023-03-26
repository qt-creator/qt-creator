// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "relationitem.h"

QT_BEGIN_NAMESPACE
class QGraphicsSimpleTextItem;
QT_END_NAMESPACE

namespace qmt {

class DConnection;
class DConnectionEnd;

class ConnectionItem : public RelationItem
{
public:
    ConnectionItem(DConnection *connection, DiagramSceneModel *diagramSceneModel,
                    QGraphicsItem *parent = nullptr);
    ~ConnectionItem() override;

protected:
    void update(const Style *style) override;

private:
    void updateEndLabels(const DConnectionEnd &end, const DConnectionEnd &otherEnd,
                         QGraphicsSimpleTextItem **endName,
                         QGraphicsSimpleTextItem **endCardinality, const Style *style);
    void placeEndLabels(const QLineF &lineSegment, QGraphicsItem *endName,
                        QGraphicsItem *endCardinality,
                        QGraphicsItem *endItem, double headLength);

    DConnection *m_connection = nullptr;
    QGraphicsSimpleTextItem *m_endAName = nullptr;
    QGraphicsSimpleTextItem *m_endACardinality = nullptr;
    QGraphicsSimpleTextItem *m_endBName = nullptr;
    QGraphicsSimpleTextItem *m_endBCardinality = nullptr;
};

} // namespace qmt
