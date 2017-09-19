/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <qmldesignercorelib_global.h>
#include <abstractview.h>
#include <QPointF>
#include <QPointer>

#pragma once

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT SelectionContext {

public:
    SelectionContext();
    SelectionContext(AbstractView *view);

    void setTargetNode(const ModelNode &modelNode);
    ModelNode targetNode() const;

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

private:
    QPointer<AbstractView> m_view;
    ModelNode m_targetNode;
    QPointF m_scenePosition;
    bool m_showSelectionTools = false;
    bool m_toggled = false;
};

} //QmlDesigner
