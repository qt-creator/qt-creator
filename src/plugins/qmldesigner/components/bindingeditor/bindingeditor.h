// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Q_PROPERTY(QString stateNameProperty READ stateName WRITE setStateName)

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
    //State Name is not mandatory, but used in bindingEditor dialog name
    void setStateModelNode(const QVariant &stateModelNode);
    void setStateName(const QString &name);

    //3. modelnode + backend value type name + optional target name
    void setModelNode(const ModelNode &modelNode);
    void setBackendValueType(const NodeMetaInfo &backendValueType);
    void setTargetName(const QString &target);

    Q_INVOKABLE void prepareBindings();
    Q_INVOKABLE void updateWindowName();

    QString targetName() const;
    QString stateName() const;

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
    NodeMetaInfo m_backendValueType;
    QString m_targetName;
};

}

QML_DECLARE_TYPE(QmlDesigner::BindingEditor)

#endif //BINDINGEDITOR_H
