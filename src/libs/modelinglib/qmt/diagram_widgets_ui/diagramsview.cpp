/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "diagramsview.h"

#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/diagram_widgets_ui/diagramview.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/model_ui/treemodel.h"

#include "qmt/diagram_controller/diagramcontroller.h"

#include "qmt/model/mdiagram.h"

namespace qmt {

DiagramsView::DiagramsView(QWidget *parent)
    : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(false);
    connect(this, &QTabWidget::currentChanged, this, &DiagramsView::onCurrentChanged);
    connect(this, &QTabWidget::tabCloseRequested, this, &DiagramsView::onTabCloseRequested);
}

DiagramsView::~DiagramsView()
{
}

void DiagramsView::setDiagramsManager(DiagramsManager *diagramsManager)
{
    m_diagramsManager = diagramsManager;
}

void DiagramsView::openDiagram(MDiagram *diagram)
{
    QMT_ASSERT(diagram, return);
    DiagramView *diagramView = m_diagramViews.value(diagram->uid());
    if (!diagramView) {
        DiagramSceneModel *diagramSceneModel = m_diagramsManager->bindDiagramSceneModel(diagram);
        auto diagramView = new DiagramView(this);
        diagramView->setDiagramSceneModel(diagramSceneModel);
        int tabIndex = addTab(diagramView, diagram->name());
        setCurrentIndex(tabIndex);
        m_diagramViews.insert(diagram->uid(), diagramView);
    } else {
        setCurrentWidget(diagramView);
    }
    emit someDiagramOpened(!m_diagramViews.isEmpty());
}

void DiagramsView::closeDiagram(const MDiagram *diagram)
{
    if (!diagram)
        return;

    DiagramView *diagramView = m_diagramViews.value(diagram->uid());
    if (diagramView) {
        removeTab(indexOf(diagramView));
        delete diagramView;
        m_diagramViews.remove(diagram->uid());
    }
    emit someDiagramOpened(!m_diagramViews.isEmpty());
}

void DiagramsView::closeAllDiagrams()
{
    for (int i = count() - 1; i >= 0; --i) {
        auto diagramView = dynamic_cast<DiagramView *>(widget(i));
        if (diagramView) {
            removeTab(i);
            delete diagramView;
        }
    }
    m_diagramViews.clear();
    emit someDiagramOpened(!m_diagramViews.isEmpty());
}

void DiagramsView::onCurrentChanged(int tabIndex)
{
    emit currentDiagramChanged(diagram(tabIndex));
}

void DiagramsView::onTabCloseRequested(int tabIndex)
{
    emit diagramCloseRequested(diagram(tabIndex));
}

void DiagramsView::onDiagramRenamed(const MDiagram *diagram)
{
    if (!diagram)
        return;
    DiagramView *diagramView = m_diagramViews.value(diagram->uid());
    if (diagramView)
        setTabText(indexOf(diagramView), diagram->name());
}

MDiagram *DiagramsView::diagram(int tabIndex) const
{
    auto diagramView = dynamic_cast<DiagramView *>(widget(tabIndex));
    return diagram(diagramView);
}

MDiagram *DiagramsView::diagram(DiagramView *diagramView) const
{
    if (!diagramView || diagramView->diagramSceneModel())
        return nullptr;
    return diagramView->diagramSceneModel()->diagram();
}

} // namespace qmt
