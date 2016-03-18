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

#include <QMap>
#include <QObject>

#include "modelnode.h"
#include "modelnodepositionstorage.h"
#include "textmodifier.h"

namespace QmlDesigner {
namespace Internal {

class ModelNodePositionRecalculator: public QObject
{
    Q_OBJECT

public:
    ModelNodePositionRecalculator(ModelNodePositionStorage *positionStore, const QList<ModelNode> &nodesToTrack):
            m_positionStore(positionStore), m_nodesToTrack(nodesToTrack)
    { Q_ASSERT(positionStore); }

    void connectTo(TextModifier *textModifier);

    QMap<int,int> dirtyAreas() const
    { return m_dirtyAreas; }

public slots:
    void replaced(int offset, int oldLength, int newLength);
    void moved(const TextModifier::MoveInfo &moveInfo);

private:
    ModelNodePositionStorage *m_positionStore;
    QList<ModelNode> m_nodesToTrack;
    QMap<int, int> m_dirtyAreas;
};

} // namespace Internal
} // namespace QmlDesigner
