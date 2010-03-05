/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "formeditorwidget.h"

#include <QWheelEvent>
#include <cmath>
#include <QCoreApplication>
#include <QPushButton>
#include <QFile>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QGraphicsView>
#include <toolbox.h>
#include <zoomaction.h>
#include <formeditorgraphicsview.h>
#include <formeditorscene.h>
#include <formeditorview.h>
#include "numberseriesaction.h"

namespace QmlDesigner {

FormEditorWidget::FormEditorWidget(FormEditorView *view)
    : QWidget(),
    m_formEditorView(view)
{
    QFile file(":/qmldesigner/stylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QVBoxLayout *fillLayout = new QVBoxLayout(this);
    fillLayout->setMargin(0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    QList<QAction*> upperActions;

    m_toolActionGroup = new QActionGroup(this);

    m_transformToolAction = m_toolActionGroup->addAction("Transform Tool (Press Key Q)");
    m_transformToolAction->setShortcut(Qt::Key_Q);
    m_transformToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_transformToolAction->setCheckable(true);
    m_transformToolAction->setChecked(true);
    m_transformToolAction->setIcon(QPixmap(":/icon/tool/transform.png"));
    connect(m_transformToolAction.data(), SIGNAL(triggered(bool)), SLOT(changeTransformTool(bool)));

    m_anchorToolAction = m_toolActionGroup->addAction("Anchor Tool (Press Key W)");
    m_anchorToolAction->setShortcut(Qt::Key_W);
    m_anchorToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_anchorToolAction->setCheckable(true);
    m_anchorToolAction->setIcon(QPixmap(":/icon/tool/anchor.png"));
    connect(m_anchorToolAction.data(), SIGNAL(triggered(bool)), SLOT(changeAnchorTool(bool)));

//    addActions(m_toolActionGroup->actions());
//    upperActions.append(m_toolActionGroup->actions());

    QAction *separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
//    addAction(separatorAction);
//    upperActions.append(separatorAction);

    QActionGroup *layoutActionGroup = new QActionGroup(this);
    layoutActionGroup->setExclusive(true);

    m_snappingAction = layoutActionGroup->addAction("Toogle Snapping (Press Key E)");
    m_snappingAction->setShortcut(Qt::Key_E);
    m_snappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingAction->setCheckable(true);
    m_snappingAction->setChecked(true);
    m_snappingAction->setIcon(QPixmap(":/icon/layout/snapping.png"));

    m_snappingAndAnchoringAction = layoutActionGroup->addAction("Toogle Snapping And Anchoring (Press Key R)");
    m_snappingAndAnchoringAction->setShortcut(Qt::Key_R);
    m_snappingAndAnchoringAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingAndAnchoringAction->setCheckable(true);
    m_snappingAndAnchoringAction->setChecked(false);
    m_snappingAndAnchoringAction->setEnabled(false);
    m_snappingAndAnchoringAction->setVisible(false);
    m_snappingAndAnchoringAction->setIcon(QPixmap(":/icon/layout/snapping_and_anchoring.png"));

    m_noSnappingAction = layoutActionGroup->addAction("Toogle Snapping And Anchoring (Press Key T)");
    m_noSnappingAction->setShortcut(Qt::Key_T);
    m_noSnappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_noSnappingAction->setCheckable(true);
    m_noSnappingAction->setChecked(false);
    m_noSnappingAction->setIcon(QPixmap(":/icon/layout/no_snapping.png"));

    addActions(layoutActionGroup->actions());
    upperActions.append(layoutActionGroup->actions());

    separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    addAction(separatorAction);
    upperActions.append(separatorAction);

    m_showBoundingRectAction = new QAction("Toogle Bounding Rectangles (Press Key A)", this);
    m_showBoundingRectAction->setShortcut(Qt::Key_A);
    m_showBoundingRectAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_showBoundingRectAction->setCheckable(true);
    m_showBoundingRectAction->setChecked(true);
    m_showBoundingRectAction->setIcon(QPixmap(":/icon/layout/boundingrect.png"));

    addAction(m_showBoundingRectAction.data());
    upperActions.append(m_showBoundingRectAction.data());

    m_selectOnlyContentItemsAction = new QAction("Select Only Items with Content (Press Key S)", this);
    m_selectOnlyContentItemsAction->setShortcut(Qt::Key_S);
    m_selectOnlyContentItemsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_selectOnlyContentItemsAction->setCheckable(true);
    m_selectOnlyContentItemsAction->setChecked(true);
    m_selectOnlyContentItemsAction->setIcon(QPixmap(":/icon/selection/selectonlycontentitems.png"));

    addAction(m_selectOnlyContentItemsAction.data());
    upperActions.append(m_selectOnlyContentItemsAction.data());


    ToolBox *upperToolBox = new ToolBox(this);
    upperToolBox->setActions(upperActions);
    fillLayout->addWidget(upperToolBox);

    m_graphicsView = new FormEditorGraphicsView(this);
    fillLayout->addWidget(m_graphicsView.data());

    QList<QAction*> lowerActions;

    m_zoomAction = new ZoomAction(toolActionGroup());
    connect(m_zoomAction.data(), SIGNAL(zoomLevelChanged(double)), SLOT(setZoomLevel(double)));
    addAction(m_zoomAction.data());
    lowerActions.append(m_zoomAction.data());

    QActionGroup *layoutMarginActionGroup = new QActionGroup(this);

    m_snappingMarginAction = new NumberSeriesAction(layoutMarginActionGroup);
    m_snappingMarginAction->addEntry("no margins (0)", 0);
    m_snappingMarginAction->addEntry("small margin (2)", 2);
    m_snappingMarginAction->addEntry("medium margin (6)", 6);
    m_snappingMarginAction->addEntry("large margin (10)", 10);
    m_snappingMarginAction->setCurrentEntryIndex(2);
    layoutMarginActionGroup->addAction(m_snappingMarginAction.data());


    m_snappingSpacingAction = new NumberSeriesAction(layoutMarginActionGroup);
    m_snappingSpacingAction->addEntry("no spacing (0)", 0);
    m_snappingSpacingAction->addEntry("small spacing (2)", 2);
    m_snappingSpacingAction->addEntry("medium spacing (4)", 4);
    m_snappingSpacingAction->addEntry("large spacing (6)", 6);
    m_snappingSpacingAction->setCurrentEntryIndex(1);
    layoutMarginActionGroup->addAction(m_snappingSpacingAction.data());

    addActions(layoutMarginActionGroup->actions());
    lowerActions.append(layoutMarginActionGroup->actions());

    m_lowerToolBox = new ToolBox(this);
    lowerToolBox()->setActions(lowerActions);
    fillLayout->addWidget(lowerToolBox());
}

void FormEditorWidget::enterEvent(QEvent *event)
{
    m_graphicsView->setFocus();
    QWidget::enterEvent(event);
}

void FormEditorWidget::changeTransformTool(bool checked)
{
    if (checked)
        m_formEditorView->changeToTransformTools();
}

void FormEditorWidget::changeAnchorTool(bool checked)
{
    if (checked && m_formEditorView->currentState().isBaseState())
        m_formEditorView->changeToAnchorTool();
}

void FormEditorWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->delta() > 0) {
            zoomAction()->zoomIn();
        } else {
            zoomAction()->zoomOut();
        }

        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }

}

ZoomAction *FormEditorWidget::zoomAction() const
{
    return m_zoomAction.data();
}

QAction *FormEditorWidget::anchorToolAction() const
{
    return m_anchorToolAction.data();
}

QAction *FormEditorWidget::transformToolAction() const
{
    return m_transformToolAction.data();
}

QAction *FormEditorWidget::showBoundingRectAction() const
{
    return m_showBoundingRectAction.data();
}

QAction *FormEditorWidget::selectOnlyContentItemsAction() const
{
    return m_selectOnlyContentItemsAction.data();
}

QAction *FormEditorWidget::snappingAction() const
{
    return m_snappingAction.data();
}

QAction *FormEditorWidget::snappingAndAnchoringAction() const
{
    return m_snappingAndAnchoringAction.data();
}

void FormEditorWidget::setZoomLevel(double zoomLevel)
{
    m_graphicsView->resetTransform();

    m_graphicsView->scale(zoomLevel, zoomLevel);
}

void FormEditorWidget::setScene(FormEditorScene *scene)
{
    m_graphicsView->setScene(scene);
}

QActionGroup *FormEditorWidget::toolActionGroup() const
{
    return m_toolActionGroup.data();
}

ToolBox *FormEditorWidget::lowerToolBox() const
{
     return m_lowerToolBox.data();
}

double FormEditorWidget::spacing() const
{
    return m_snappingSpacingAction->currentValue().toDouble();
}

double FormEditorWidget::margins() const
{
    return m_snappingMarginAction->currentValue().toDouble();
}

void FormEditorWidget::setFeedbackNode(const QmlItemNode &node)
{
    m_graphicsView->setFeedbackNode(node);
}

}


