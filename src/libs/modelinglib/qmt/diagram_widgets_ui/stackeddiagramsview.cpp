// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackeddiagramsview.h"

#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/diagram_widgets_ui/diagramview.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/model_ui/treemodel.h"

#include "qmt/diagram_controller/diagramcontroller.h"

#include "qmt/model/mdiagram.h"

namespace qmt {

StackedDiagramsView::StackedDiagramsView(QWidget *parent)
    : QStackedWidget(parent)
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
    QMT_ASSERT(diagram, return);
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
    Q_UNUSED(diagram)

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
        return nullptr;
    return diagramView->diagramSceneModel()->diagram();
}

} // namespace qmt
