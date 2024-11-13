// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>
#include <QPointer>

namespace QmlDesigner {

class NodeGraphEditorView;

class NodeGraphEditorModel : public QStandardItemModel
{
    Q_OBJECT

public:
    NodeGraphEditorModel(NodeGraphEditorView *nodeGraphEditorView);

private:
    QPointer<NodeGraphEditorView> m_editorView;
};

} // namespace QmlDesigner
