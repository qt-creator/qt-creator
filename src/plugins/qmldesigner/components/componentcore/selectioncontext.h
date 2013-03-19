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

#include <qmldesignercorelib_global.h>
#include <qmlmodelview.h>

#ifndef SELECTIONCONTEXT_H
#define SELECTIONCONTEXT_H

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT SelectionContext {

public:
    SelectionContext();
    SelectionContext(QmlModelView *qmlModelView);

    void setTargetNode(const ModelNode &modelNode);
    ModelNode targetNode() const;

    bool singleNodeIsSelected() const;
    bool isInBaseState() const;

    ModelNode currentSingleSelectedNode() const;
    QList<ModelNode> selectedModelNodes() const;

    QmlModelView *qmlModelView() const;

    void setShowSelectionTools(bool show);
    bool showSelectionTools() const;

    void setScenePos(const QPoint &postition);
    QPoint scenePos() const;

    void setToggled(bool toggled);
    bool toggled() const;

    bool isValid() const;

private:
    QWeakPointer<QmlModelView> m_qmlModelView;
    ModelNode m_targetNode;
    bool m_isInBaseState;
    QList<ModelNode> m_selectedModelNodes;
    bool m_showSelectionTools;
    QPoint m_scenePosition;
    bool m_toggled;
};

} //QmlDesigner

#endif //SELECTIONCONTEXT_H
