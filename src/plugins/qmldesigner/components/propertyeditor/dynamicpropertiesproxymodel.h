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

#include "propertyeditorvalue.h"

#include <abstractview.h>
#include <qmlitemnode.h>

#include <QAbstractListModel>
#include <QColor>
#include <QtQml>

namespace QmlDesigner {

class DynamicPropertiesModel;

class DynamicPropertiesProxyModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DynamicPropertiesProxyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    static void registerDeclarativeType();

    DynamicPropertiesModel *dynamicPropertiesModel() const;

    Q_INVOKABLE QString newPropertyName() const;
    Q_INVOKABLE void createProperty(const QString &name, const QString &type);

protected:
    void initModel(DynamicPropertiesModel *model);

private:
    DynamicPropertiesModel *m_model = nullptr;
};

class DynamicPropertyRow : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged FINAL)
    Q_PROPERTY(QmlDesigner::PropertyEditorValue *backendValue READ backendValue NOTIFY rowChanged FINAL)
    Q_PROPERTY(DynamicPropertiesProxyModel *model READ model WRITE setModel NOTIFY modelChanged FINAL)

public:
    explicit DynamicPropertyRow();
    ~DynamicPropertyRow();

    static void registerDeclarativeType();

    void setRow(int r);
    int row() const;
    void setModel(DynamicPropertiesProxyModel *model);
    DynamicPropertiesProxyModel *model() const;
    QmlDesigner::PropertyEditorValue *backendValue() const;

    Q_INVOKABLE void remove();
    Q_INVOKABLE QmlDesigner::PropertyEditorValue *createProxyBackendValue();
    Q_INVOKABLE void clearProxyBackendValues();

signals:
    void rowChanged();
    void modelChanged();

private:
    void setupBackendValue();
    void commitValue(const QVariant &value);
    void commitExpression(const QString &expression);
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex &, const QList<int> &);
    void resetValue();

    int m_row = -1;
    QmlDesigner::PropertyEditorValue *m_backendValue = nullptr;
    DynamicPropertiesProxyModel *m_model = nullptr;
    QList<QmlDesigner::PropertyEditorValue *> m_proxyBackendValues;
    bool m_lock = false;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::DynamicPropertyRow)
