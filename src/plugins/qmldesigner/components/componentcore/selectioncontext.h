// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <abstractview.h>
#include <QPointF>
#include <QPointer>
#include <qmldesignercomponents_global.h>

#pragma once

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT SelectionContext
{
public:
    enum class UpdateMode {
        Normal,
        Fast,
        Properties,
        NodeCreated,
        NodeHierachy,
        Selection
    };

    SelectionContext();
    SelectionContext(AbstractView *view);

    void setTargetNode(const ModelNode &modelNode);
    ModelNode targetNode() const;
    ModelNode rootNode() const;

    bool singleNodeIsSelected() const;
    bool isInBaseState() const;

    ModelNode currentSingleSelectedNode() const;
    ModelNode firstSelectedModelNode() const;
    QList<ModelNode> selectedModelNodes() const;
    bool hasSingleSelectedModelNode() const;

    AbstractView *view() const;

    void setShowSelectionTools(bool show);
    bool showSelectionTools() const;

    void setScenePosition(const QPointF &postition);
    QPointF scenePosition() const;

    void setToggled(bool toggled);
    bool toggled() const;

    bool isValid() const;

    bool fastUpdate() const;
    void setUpdateMode(UpdateMode mode);

    UpdateMode updateReason() const;

private:
    QPointer<AbstractView> m_view;
    ModelNode m_targetNode;
    QPointF m_scenePosition;
    bool m_showSelectionTools = false;
    bool m_toggled = false;
    UpdateMode m_updateReason = UpdateMode::Normal;
};

} //QmlDesigner
