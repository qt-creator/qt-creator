/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include <abstractview.h>
#include <QPointF>
#include <QPointer>

#ifndef SELECTIONCONTEXT_H
#define SELECTIONCONTEXT_H

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
    bool m_showSelectionTools;
    QPointF m_scenePosition;
    bool m_toggled;
};

} //QmlDesigner

#endif //SELECTIONCONTEXT_H
