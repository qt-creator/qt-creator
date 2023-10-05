// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ACTIONEDITOR_H
#define ACTIONEDITOR_H

#include <bindingeditor/actioneditordialog.h>
#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include <signalhandlerproperty.h>

#include <QtQml>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ActionEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ connectionValue WRITE setConnectionValue)

public:
    ActionEditor(QObject *parent = nullptr);
    ~ActionEditor();

    static void registerDeclarativeType();

    Q_INVOKABLE void showWidget();
    Q_INVOKABLE void showWidget(int x, int y);
    Q_INVOKABLE void hideWidget();

    Q_INVOKABLE void showControls(bool show);
    Q_INVOKABLE void setMultilne(bool multiline);

    QString connectionValue() const;
    void setConnectionValue(const QString &text);

    QString rawConnectionValue() const;

    bool hasModelIndex() const;
    void resetModelIndex();
    QModelIndex modelIndex() const;
    void setModelIndex(const QModelIndex &index);

    void setModelNode(const ModelNode &modelNode);

    void prepareConnections();

    Q_INVOKABLE void updateWindowName(const QString &targetName = {});

    static void invokeEditor(SignalHandlerProperty signalHandler,
                             std::function<void(SignalHandlerProperty)> removeSignalFunction = nullptr,
                             bool removeOnReject = false,
                             QObject *parent = nullptr);

signals:
    void accepted();
    void rejected();

private:
    void prepareDialog();

private:
    QPointer<ActionEditorDialog> m_dialog;
    QModelIndex m_index;
    QmlDesigner::ModelNode m_modelNode;
};

}

QML_DECLARE_TYPE(QmlDesigner::ActionEditor)

#endif //ACTIONEDITOR_H
