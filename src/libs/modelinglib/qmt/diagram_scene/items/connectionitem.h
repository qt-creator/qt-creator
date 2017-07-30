/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
