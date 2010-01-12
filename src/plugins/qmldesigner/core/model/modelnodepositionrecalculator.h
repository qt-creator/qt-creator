/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MODELNODEPOSITIONRECALCULATOR_H
#define MODELNODEPOSITIONRECALCULATOR_H

#include <QMap>
#include <QObject>
#include <QSet>

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

#endif // MODELNODEPOSITIONRECALCULATOR_H
