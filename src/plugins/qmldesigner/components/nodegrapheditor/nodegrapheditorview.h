// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

namespace QmlDesigner {

class NodeGraphEditorModel;
class NodeGraphEditorWidget;

class NodeGraphEditorView : public AbstractView {
    Q_OBJECT

public:
    explicit NodeGraphEditorView(ExternalDependenciesInterface &externalDependencies);
    ~NodeGraphEditorView() override;

    bool hasWidget() const override { return true; }
    WidgetInfo widgetInfo() override;
    void modelAttached(QmlDesigner::Model * model) override;

private:
  void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<QmlDesigner::ModelNode> &nodeList, const QList<QVariant> &data) override;
    QPointer<NodeGraphEditorModel> m_editorModel;
    QPointer<NodeGraphEditorWidget> m_editorWidget;
    QString m_currProjectPath;
};

} // namespace QmlDesigner
