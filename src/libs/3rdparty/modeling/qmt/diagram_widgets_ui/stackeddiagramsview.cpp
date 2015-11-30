/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stackeddiagramsview.h"

#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/diagram_widgets_ui/diagramview.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/model_ui/treemodel.h"

#include "qmt/diagram_controller/diagramcontroller.h"

#include "qmt/model/mdiagram.h"

namespace qmt {

StackedDiagramsView::StackedDiagramsView(QWidget *parent)
    : QStackedWidget(parent),
      m_diagramsManager(0)
{
    connect(this, &QStackedWidget::currentChanged, this, &StackedDiagramsView::onCurrentChanged);
}

StackedDiagramsView::~StackedDiagramsView()
{
}

void StackedDiagramsView::setDiagramsManager(DiagramsManager *diagramsManager)
{
    m_diagramsManager = diagramsManager;
}

void StackedDiagramsView::openDiagram(MDiagram *diagram)
{
    QMT_CHECK(diagram);
    DiagramView *diagramView = m_diagramViews.value(diagram->uid());
    if (!diagramView) {
        DiagramSceneModel *diagramSceneModel = m_diagramsManager->bindDiagramSceneModel(diagram);
        auto diagramView = new DiagramView(this);
        diagramView->setDiagramSceneModel(diagramSceneModel);
        int tabIndex = addWidget(diagramView);
        setCurrentIndex(tabIndex);
        m_diagramViews.insert(diagram->uid(), diagramView);
    } else {
        setCurrentWidget(diagramView);
    }
    emit someDiagramOpened(!m_diagramViews.isEmpty());
}

void StackedDiagramsView::closeDiagram(const MDiagram *diagram)
{
    if (!diagram)
        return;

    DiagramView *diagramView = m_diagramViews.value(diagram->uid());
    if (diagramView) {
        removeWidget(diagramView);
        delete diagramView;
        m_diagramViews.remove(diagram->uid());
    }
    emit someDiagramOpened(!m_diagramViews.isEmpty());
}

void StackedDiagramsView::closeAllDiagrams()
{
    for (int i = count() - 1; i >= 0; --i) {
        auto diagramView = dynamic_cast<DiagramView *>(widget(i));
        if (diagramView) {
            removeWidget(diagramView);
            delete diagramView;
        }
    }
    m_diagramViews.clear();
    emit someDiagramOpened(!m_diagramViews.isEmpty());
}

void StackedDiagramsView::onCurrentChanged(int tabIndex)
{
    emit currentDiagramChanged(diagram(tabIndex));
}

void StackedDiagramsView::onDiagramRenamed(const MDiagram *diagram)
{
    Q_UNUSED(diagram);

    // nothing to do!
}

MDiagram *StackedDiagramsView::diagram(int tabIndex) const
{
    auto diagramView = dynamic_cast<DiagramView *>(widget(tabIndex));
    return diagram(diagramView);
}

MDiagram *StackedDiagramsView::diagram(DiagramView *diagramView) const
{
    if (!diagramView || !diagramView->diagramSceneModel())
        return 0;
    return diagramView->diagramSceneModel()->diagram();
}

} // namespace qmt
