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

#include "tracing_global.h"
#include "flamegraphattached.h"

#include <QQuickItem>
#include <QAbstractItemModel>

namespace FlameGraph {

class TRACING_EXPORT FlameGraph : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int sizeRole READ sizeRole WRITE setSizeRole NOTIFY sizeRoleChanged)
    Q_PROPERTY(qreal sizeThreshold READ sizeThreshold WRITE setSizeThreshold
               NOTIFY sizeThresholdChanged)
    Q_PROPERTY(int maximumDepth READ maximumDepth WRITE setMaximumDepth
               NOTIFY maximumDepthChanged)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged)
    Q_PROPERTY(QPersistentModelIndex root READ root WRITE setRoot NOTIFY rootChanged)
    Q_PROPERTY(bool zoomed READ isZoomed NOTIFY rootChanged)
    Q_PROPERTY(int selectedTypeId READ selectedTypeId WRITE setSelectedTypeId
               NOTIFY selectedTypeIdChanged)

public:
    FlameGraph(QQuickItem *parent = nullptr);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    int sizeRole() const;
    void setSizeRole(int sizeRole);

    qreal sizeThreshold() const;
    void setSizeThreshold(qreal sizeThreshold);

    int depth() const;

    int maximumDepth() const
    {
        return m_maximumDepth;
    }

    void setMaximumDepth(int maximumDepth)
    {
        if (maximumDepth != m_maximumDepth) {
            m_maximumDepth = maximumDepth;
            emit maximumDepthChanged();
        }
    }

    QPersistentModelIndex root() const
    {
        return m_root;
    }

    bool isZoomed() const
    {
        return m_root.isValid();
    }

    void setRoot(const QPersistentModelIndex &root)
    {
        if (root != m_root) {
            m_root = root;
            emit rootChanged(root);
        }
    }

    Q_INVOKABLE void resetRoot()
    {
        setRoot(QModelIndex());
    }

    Q_INVOKABLE QVariant total(int role) const
    {
        return m_model ? m_model->data(m_root, role) : QVariant();
    }

    static FlameGraphAttached *qmlAttachedProperties(QObject *object);

    int selectedTypeId() const
    {
        return m_selectedTypeId;
    }

    void setSelectedTypeId(int selectedTypeId)
    {
        if (m_selectedTypeId == selectedTypeId)
            return;

        m_selectedTypeId = selectedTypeId;
        emit selectedTypeIdChanged(m_selectedTypeId);
    }

signals:
    void delegateChanged(QQmlComponent *delegate);
    void modelChanged(QAbstractItemModel *model);
    void sizeRoleChanged(int role);
    void sizeThresholdChanged(qreal threshold);
    void depthChanged(int depth);
    void maximumDepthChanged();
    void rootChanged(const QPersistentModelIndex &root);
    void selectedTypeIdChanged(int selectedTypeId);

private:
    void rebuild();

    QQmlComponent *m_delegate = nullptr;
    QAbstractItemModel *m_model = nullptr;
    QPersistentModelIndex m_root;
    int m_sizeRole = 0;
    int m_depth = 0;
    qreal m_sizeThreshold = 0;
    int m_maximumDepth = std::numeric_limits<int>::max();
    int m_selectedTypeId = -1;

    int buildNode(const QModelIndex &parentIndex, QObject *parentObject, int depth,
                  int maximumDepth);
    QObject *appendChild(QObject *parentObject, QQuickItem *parentItem, QQmlContext *context,
                         const QModelIndex &childIndex, qreal position, qreal size);

    void mousePressEvent(QMouseEvent *event) final;
    void mouseReleaseEvent(QMouseEvent *event) final;
    void mouseDoubleClickEvent(QMouseEvent *event) final;
};

} // namespace FlameGraph

QML_DECLARE_TYPEINFO(FlameGraph::FlameGraph, QML_HAS_ATTACHED_PROPERTIES)
