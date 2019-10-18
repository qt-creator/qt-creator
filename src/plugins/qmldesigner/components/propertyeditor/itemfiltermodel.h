/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <qmlitemnode.h>

#include <QDir>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml>

class ItemFilterModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList itemModel READ itemModel NOTIFY itemModelChanged)
    Q_PROPERTY(bool selectionOnly READ selectionOnly WRITE setSelectionOnly NOTIFY selectionOnlyChanged)

public:
    explicit ItemFilterModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void setTypeFilter(const QString &typeFilter);
    void setSelectionOnly(bool value);
    QString typeFilter() const;
    bool selectionOnly() const;
    void setupModel();
    QStringList itemModel() const;

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();
    void itemModelChanged();
    void selectionOnlyChanged();

private:
    QVariant modelNodeBackend() const;

private:
    QString m_typeFilter;
    bool m_lock;
    QStringList m_model;
    QmlDesigner::ModelNode m_modelNode;
    bool m_selectionOnly;
};

QML_DECLARE_TYPE(ItemFilterModel)
