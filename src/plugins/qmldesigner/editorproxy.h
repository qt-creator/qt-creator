// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QQmlEngine>
#include <QPointer>

#include "modelnode.h"

namespace QmlDesigner {
class EditorProxy : public QObject
{
    Q_OBJECT
public:
    EditorProxy(QObject *parent = nullptr);
    ~EditorProxy();

    Q_INVOKABLE virtual void showWidget();
    Q_INVOKABLE void showWidget(int x, int y);
    Q_INVOKABLE virtual void hideWidget();

    QWidget *widget() const;
    virtual QWidget *createWidget() = 0;

    template<typename T>
    static void registerType(const char *className)
    {
        qmlRegisterType<T>("HelperWidgets", 2, 0, className);
    }

protected:
    QPointer<QWidget> m_widget;
};

class ModelNodeEditorProxy : public EditorProxy
{
    Q_OBJECT
    Q_PROPERTY(bool hasCustomId READ hasCustomId NOTIFY customIdChanged)
    Q_PROPERTY(bool hasAnnotation READ hasAnnotation NOTIFY annotationChanged)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)
public:
    ModelNodeEditorProxy(QObject *parent = nullptr);
    ~ModelNodeEditorProxy();

    ModelNode modelNode() const;
    virtual void setModelNode(const ModelNode &modelNode);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    QVariant modelNodeBackend() const;

    Q_INVOKABLE bool hasCustomId() const;
    Q_INVOKABLE bool hasAnnotation() const;

    template<typename T>
    static T *fromModelNode(const ModelNode &modelNode, QVariant const &modelNodeBackend = {})
    {
        auto *editor = new T;
        editor->setModelNode(modelNode);
        if (!modelNodeBackend.isNull())
            editor->setModelNodeBackend(modelNodeBackend);

        editor->showWidget();
        if (editor->m_widget) {
            connect(editor->m_widget, &QObject::destroyed, [editor]() { editor->deleteLater(); });
        }
        return editor;
    }

signals:
    void customIdChanged();
    void annotationChanged();
    void modelNodeBackendChanged();

protected:
    QVariant m_modelNodeBackend;
    ModelNode m_modelNode;
};
} // namespace QmlDesigner
