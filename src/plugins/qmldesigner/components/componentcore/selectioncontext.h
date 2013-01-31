/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <qmlmodelview.h>

#ifndef SELECTIONCONTEXT_H
#define SELECTIONCONTEXT_H

namespace QmlDesigner {

class SelectionContext {

public:
    SelectionContext();
    SelectionContext(QmlModelView *view);

    void setTargetNode(const ModelNode &modelNode)
    { m_targetNode = modelNode; }

    bool singleSelected() const
    { return m_singleSelected; }

    bool isInBaseState() const
    { return m_isInBaseState; }

    ModelNode targetNode() const
    { return m_targetNode; }

    ModelNode currentSingleSelectedNode() const
    { return m_currentSingleSelectedNode; }

    QList<ModelNode> selectedModelNodes() const
    { return m_selectedModelNodes; }

    QmlModelView *view() const
    { return m_view; }

    void setShowSelectionTools(bool show)
    { m_showSelectionTools = show; }

    bool showSelectionTools() const
    { return m_showSelectionTools; }

    QPoint scenePos() const
    { return m_scenePos; }

    void setScenePos(const QPoint &pos)
    { m_scenePos = pos; }

    void setToggled(bool b)
    { m_toggled = b; }

    bool toggled() const
    { return m_toggled; }

    bool isValid() const
    { return view() && view()->model() && view()->nodeInstanceView(); }

private:
    QmlModelView *m_view;
    bool m_singleSelected;
    ModelNode m_currentSingleSelectedNode;
    ModelNode m_targetNode;
    bool m_isInBaseState;
    QList<ModelNode> m_selectedModelNodes;
    bool m_showSelectionTools;
    QPoint m_scenePos;
    bool m_toggled;
};

} //QmlDesigner

#endif //SELECTIONCONTEXT_H
