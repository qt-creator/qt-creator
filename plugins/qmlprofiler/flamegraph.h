/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef FLAMEGRAPH_H
#define FLAMEGRAPH_H

#include <QQuickItem>
#include <QAbstractItemModel>

namespace QmlProfilerExtension {
namespace Internal {

class FlameGraphAttached : public QObject
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

    Q_INVOKABLE QVariant data(int role) const;

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

class FlameGraph : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int sizeRole READ sizeRole WRITE setSizeRole NOTIFY sizeRoleChanged)
    Q_PROPERTY(qreal sizeThreshold READ sizeThreshold WRITE setSizeThreshold
               NOTIFY sizeThresholdChanged)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged)

public:
    FlameGraph(QQuickItem *parent = 0);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    int sizeRole() const;
    void setSizeRole(int sizeRole);

    qreal sizeThreshold() const;
    void setSizeThreshold(qreal sizeThreshold);

    int depth() const;

    static FlameGraphAttached *qmlAttachedProperties(QObject *object);

signals:
    void delegateChanged(QQmlComponent *delegate);
    void modelChanged(QAbstractItemModel *model);
    void sizeRoleChanged(int role);
    void sizeThresholdChanged(qreal threshold);
    void depthChanged(int depth);

private slots:
    void rebuild();

private:
    QQmlComponent *m_delegate;
    QAbstractItemModel *m_model;
    int m_sizeRole;
    int m_depth;
    qreal m_sizeThreshold;

    int buildNode(const QModelIndex &parentIndex, QObject *parentObject, int depth);
    QObject *appendChild(QObject *parentObject, QQuickItem *parentItem, QQmlContext *context,
                         const QModelIndex &childIndex, qreal position, qreal size);
};

}
}

QML_DECLARE_TYPEINFO(QmlProfilerExtension::Internal::FlameGraph, QML_HAS_ATTACHED_PROPERTIES)

#endif // FLAMEGRAPH_H
