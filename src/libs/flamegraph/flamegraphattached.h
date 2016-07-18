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

#include "flamegraph_global.h"
#include <QObject>
#include <QModelIndex>
#include <QVariant>

namespace FlameGraph {

class FLAMEGRAPH_EXPORT FlameGraphAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal relativeSize READ relativeSize WRITE setRelativeSize
               NOTIFY relativeSizeChanged)
    Q_PROPERTY(qreal relativePosition READ relativePosition WRITE setRelativePosition
               NOTIFY relativePositionChanged)
    Q_PROPERTY(bool dataValid READ isDataValid NOTIFY dataValidChanged)

public:
    FlameGraphAttached(QObject *parent = 0) :
        QObject(parent), m_relativeSize(0), m_relativePosition(0) {}

    Q_INVOKABLE QVariant data(int role) const
    {
        return m_data.isValid() ? m_data.data(role) : QVariant();
    }

    bool isDataValid() const
    {
        return m_data.isValid();
    }

    qreal relativeSize() const
    {
        return m_relativeSize;
    }

    void setRelativeSize(qreal relativeSize)
    {
        if (relativeSize != m_relativeSize) {
            m_relativeSize = relativeSize;
            emit relativeSizeChanged();
        }
    }

    qreal relativePosition() const
    {
        return m_relativePosition;
    }

    void setRelativePosition(qreal relativePosition)
    {
        if (relativePosition != m_relativePosition) {
            m_relativePosition = relativePosition;
            emit relativePositionChanged();
        }
    }

    void setModelIndex(const QModelIndex &data)
    {
        if (data != m_data) {
            bool validChanged = (data.isValid() != m_data.isValid());
            m_data = data;
            if (validChanged)
                emit dataValidChanged();
            emit dataChanged();
        }
    }

signals:
    void dataChanged();
    void dataValidChanged();
    void relativeSizeChanged();
    void relativePositionChanged();

private:
    QPersistentModelIndex m_data;
    qreal m_relativeSize;
    qreal m_relativePosition;
};

} // namespace FlameGraph
