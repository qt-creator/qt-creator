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

#ifndef BINDINGEDITOR_H
#define BINDINGEDITOR_H

#include <bindingeditor/bindingeditordialog.h>
#include <qmldesignercorelib_global.h>
#include <modelnode.h>

#include <QtQml>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class BindingEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ bindingValue WRITE setBindingValue)
    Q_PROPERTY(QVariant backendValueProperty READ backendValue WRITE setBackendValue NOTIFY backendValueChanged)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QVariant stateModelNodeProperty READ stateModelNode WRITE setStateModelNode NOTIFY stateModelNodeChanged)

public:
    BindingEditor(QObject *parent = nullptr);
    ~BindingEditor();

    static void registerDeclarativeType();

    Q_INVOKABLE void showWidget();
    Q_INVOKABLE void showWidget(int x, int y);
    Q_INVOKABLE void hideWidget();

    QString bindingValue() const;
    void setBindingValue(const QString &text);

    //there are few ways to setup backend for binding editor:
    //1. backend value + model node backend
    void setBackendValue(const QVariant &backendValue);
    void setModelNodeBackend(const QVariant &modelNodeBackend);

    //2. modelnode (this one also sets backend value type name to bool)
    void setStateModelNode(const QVariant &stateModelNode);

    //3. modelnode + backend value type name
    void setModelNode(const ModelNode &modelNode);
    void setBackendValueTypeName(const TypeName &backendValueTypeName);

    Q_INVOKABLE void prepareBindings();
    Q_INVOKABLE void updateWindowName();

signals:
    void accepted();
    void rejected();
    void backendValueChanged();
    void modelNodeBackendChanged();
    void stateModelNodeChanged();

private:
    QVariant backendValue() const;
    QVariant modelNodeBackend() const;
    QVariant stateModelNode() const;
    void prepareDialog();

private:
    QPointer<BindingEditorDialog> m_dialog;
    QVariant m_backendValue;
    QVariant m_modelNodeBackend;
    QVariant m_stateModelNode;
    QmlDesigner::ModelNode m_modelNode;
    TypeName m_backendValueTypeName;
};

}

QML_DECLARE_TYPE(QmlDesigner::BindingEditor)

#endif //BINDINGEDITOR_H
