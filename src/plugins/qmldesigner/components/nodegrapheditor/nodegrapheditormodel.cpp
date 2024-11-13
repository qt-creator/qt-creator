// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditormodel.h"

#include "nodegrapheditorview.h"

namespace QmlDesigner {

NodeGraphEditorModel::NodeGraphEditorModel(NodeGraphEditorView *nodeGraphEditorView)
    : QStandardItemModel(nodeGraphEditorView)
    , m_editorView(nodeGraphEditorView)
{
}

} // namespace QmlDesigner
