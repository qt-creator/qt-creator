// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditorview.h"
#include "nodegrapheditormodel.h"
#include "nodegrapheditorwidget.h"
#include <qmldesignerplugin.h>
namespace QmlDesigner {

NodeGraphEditorView::NodeGraphEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_editorModel(new NodeGraphEditorModel(this))
{
    Q_ASSERT(m_editorModel);
}

NodeGraphEditorView::~NodeGraphEditorView()
{
    delete m_editorWidget.data();
}


void NodeGraphEditorView::modelAttached(QmlDesigner::Model *model)
{
    AbstractView::modelAttached(model);
    QString currProjectPath = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toString();
    if (m_currProjectPath != currProjectPath) { // starting a new project
        m_editorModel->clear();
    }

    m_currProjectPath = currProjectPath;
}


WidgetInfo NodeGraphEditorView::widgetInfo()
{
    if (!m_editorWidget)
        m_editorWidget = new NodeGraphEditorWidget(this, m_editorModel.data());

    return createWidgetInfo(m_editorWidget.data(),
                            "NodeGraphEditor",
                            WidgetInfo::BottomPane,
                            tr("Node Graph"));
}

void NodeGraphEditorView::customNotification([[maybe_unused]] const AbstractView *view,
                                         const QString &identifier,
                                         [[maybe_unused]] const QList<QmlDesigner::ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    if (data.size() < 1)
        return;
    if (identifier == "open_nodegrapheditor_graph") {
        const QString nodeGraphPath = data[0].toString();
        m_editorWidget->openNodeGraph(nodeGraphPath);
    }
}
} // namespace QmlDesigner
