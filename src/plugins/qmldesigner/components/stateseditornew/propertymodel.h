/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QAbstractListModel>
#include <QPointer>

#include <modelnode.h>

namespace QmlDesigner {
namespace Experimental {

class PropertyModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(bool expanded READ expanded NOTIFY expandedChanged)

    enum { Name = Qt::DisplayRole, Value = Qt::UserRole, Type };

public:
    PropertyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setModelNodeBackend(const QVariant &modelNodeBackend);

    Q_INVOKABLE void setExplicit(bool value);
    Q_INVOKABLE void setRestoreEntryValues(bool value);
    Q_INVOKABLE void removeProperty(const QString &name);

    Q_INVOKABLE void setExpanded(bool value);
    Q_INVOKABLE bool expanded() const;

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();
    void expandedChanged();

private:
    QVariant modelNodeBackend() const;
    void setupModel();

private:
    ModelNode m_modelNode;
    QList<AbstractProperty> m_properties;
};

} // namespace Experimental
} // namespace QmlDesigner
