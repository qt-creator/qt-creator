// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include <QObject>
#include <QModelIndex>
#include <QVariant>

namespace FlameGraph {

class TRACING_EXPORT FlameGraphAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal relativeSize READ relativeSize WRITE setRelativeSize
               NOTIFY relativeSizeChanged)
    Q_PROPERTY(qreal relativePosition READ relativePosition WRITE setRelativePosition
               NOTIFY relativePositionChanged)
    Q_PROPERTY(bool dataValid READ isDataValid NOTIFY dataValidChanged)
    Q_PROPERTY(QModelIndex modelIndex READ modelIndex WRITE setModelIndex NOTIFY modelIndexChanged)

public:
    FlameGraphAttached(QObject *parent = nullptr) :
        QObject(parent), m_relativeSize(0), m_relativePosition(0) {}

    QModelIndex modelIndex() const
    {
        return m_data;
    }

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
            emit modelIndexChanged();
        }
    }

signals:
    void dataValidChanged();
    void modelIndexChanged();
    void relativeSizeChanged();
    void relativePositionChanged();

private:
    QPersistentModelIndex m_data;
    qreal m_relativeSize;
    qreal m_relativePosition;
};

} // namespace FlameGraph
