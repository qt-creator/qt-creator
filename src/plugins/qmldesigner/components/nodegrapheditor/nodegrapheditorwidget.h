// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <studioquickwidget.h>

#include <QElapsedTimer>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceView;

class AbstractView;
class NodeGraphEditorModel;
class NodeGraphEditorView;

namespace Internal { class NodeGraphEditorImageProvider; }

class NodeGraphEditorWidget : public StudioQuickWidget
{
    Q_OBJECT

public:
    NodeGraphEditorWidget(NodeGraphEditorView *nodeGraphEditorView, NodeGraphEditorModel *nodeGraphEditorModel);
    ~NodeGraphEditorWidget() override = default;

    Q_INVOKABLE QList<QVariantMap> createMetaData_inPorts(QByteArray typeName);

    Q_INVOKABLE QString generateUUID() const;
    static QString qmlSourcesPath();
    Q_INVOKABLE void doOpenNodeGraph();
    void openNodeGraph(const QString &path);

protected:
    void showEvent(QShowEvent *) override;
    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void reloadQmlSource();
private:
    QPointer<NodeGraphEditorView> m_editorView;
    QPointer<NodeGraphEditorModel> m_model;
    Internal::NodeGraphEditorImageProvider *m_imageProvider;
    QShortcut *m_qmlSourceUpdateShortcut;
    QElapsedTimer m_usageTimer;
    QString m_filePath;
};

} // namespace QmlDesigner
