// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditorview.h"

#include "nodegrapheditormodel.h"
#include "nodegrapheditorwidget.h"

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

WidgetInfo NodeGraphEditorView::widgetInfo()
{
    if (!m_editorWidget)
        m_editorWidget = new NodeGraphEditorWidget(this, m_editorModel.data());

    return createWidgetInfo(m_editorWidget.data(),
                            "NodeGraphEditor",
                            WidgetInfo::BottomPane,
                            tr("Node Graph"));
}

} // namespace QmlDesigner
