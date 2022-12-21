// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    void replaced(int offset, int oldLength, int newLength);
    void moved(const TextModifier::MoveInfo &moveInfo);

private:
    ModelNodePositionStorage *m_positionStore;
    QList<ModelNode> m_nodesToTrack;
    QMap<int, int> m_dirtyAreas;
};

} // namespace Internal
} // namespace QmlDesigner
