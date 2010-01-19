/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

    addActions(m_toolActionGroup->actions());

    QActionGroup *layoutActionGroup = new QActionGroup(this);
    layoutActionGroup->setExclusive(false);

    m_snappingToolAction = layoutActionGroup->addAction("Toogle Snapping (Press Key E)");
    m_snappingToolAction->setShortcut(Qt::Key_E);
    m_snappingToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingToolAction->setCheckable(true);
    m_snappingToolAction->setChecked(true);
    m_snappingToolAction->setIcon(QPixmap(":/icon/layout/snapping.png"));
    connect(m_snappingToolAction.data(), SIGNAL(triggered(bool)), SLOT(changeSnappingTool(bool)));

    m_showBoundingRectAction = layoutActionGroup->addAction("Toogle Bounding Rectangles (Press Key R)");
    m_showBoundingRectAction->setShortcut(Qt::Key_R);
    m_showBoundingRectAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_showBoundingRectAction->setCheckable(true);
    m_showBoundingRectAction->setChecked(true);
    m_showBoundingRectAction->setIcon(QPixmap(":/icon/layout/boundingrect.png"));


    m_snappingMarginAction = new NumberSeriesAction(layoutActionGroup);
    m_snappingMarginAction->addEntry("no margins (0)", 0);
    m_snappingMarginAction->addEntry("small margin (2)", 2);
    m_snappingMarginAction->addEntry("medium margin (6)", 6);
    m_snappingMarginAction->addEntry("all in margin (10)", 10);
    m_snappingMarginAction->setCurrentEntryIndex(3);
    layoutActionGroup->addAction(m_snappingMarginAction.data());
    addActions(layoutActionGroup->actions());

    m_snappingSpacingAction = new NumberSeriesAction(layoutActionGroup);
    m_snappingSpacingAction->addEntry("no spacing (0)", 0);
    m_snappingSpacingAction->addEntry("small spacing (2)", 2);
    m_snappingSpacingAction->addEntry("medium spacing (4)", 4);
    m_snappingSpacingAction->addEntry("all in spacing (6)", 6);
    m_snappingSpacingAction->setCurrentEntryIndex(1);
    layoutActionGroup->addAction(m_snappingSpacingAction.data());

    addActions(layoutActionGroup->actions());

    m_zoomAction = new ZoomAction(toolActionGroup());
    connect(m_zoomAction.data(), SIGNAL(zoomLevelChanged(double)), SLOT(setZoomLevel(double)));
    addAction(m_zoomAction.data());

    QAction *separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    addAction(separatorAction);

    m_selectOnlyContentItemsAction = layoutActionGroup->addAction("Select Only Items with Content (Press Key T)");
    m_selectOnlyContentItemsAction->setShortcut(Qt::Key_T);
    m_selectOnlyContentItemsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_selectOnlyContentItemsAction->setCheckable(true);
    m_selectOnlyContentItemsAction->setChecked(true);
    m_selectOnlyContentItemsAction->setIcon(QPixmap(":/icon/selection/selectonlycontentitems.png"));

    addAction(m_selectOnlyContentItemsAction.data());

    separatorAction = new QAction(toolActionGroup());
    separatorAction->setSeparator(true);
    addAction(separatorAction);

    m_toolBox = new ToolBox(this);
    toolBox()->setActions(actions());
    fillLayout->addWidget(toolBox());

    m_graphicsView = new FormEditorGraphicsView(this);
    fillLayout->addWidget(m_graphicsView.data());
}

void FormEditorWidget::enterEvent(QEvent *event)
{
    setFocus();
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

void FormEditorWidget::changeSnappingTool(bool /*checked*/)
{
    // TODO
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

ToolBox *FormEditorWidget::toolBox() const
{
     return m_toolBox.data();
}

bool FormEditorWidget::isSnapButtonChecked() const
{
    return m_snappingToolAction->isChecked();
}

double FormEditorWidget::spacing() const
{
    return m_snappingSpacingAction->currentValue().toDouble();
}

double FormEditorWidget::margins() const
{
    return m_snappingMarginAction->currentValue().toDouble();
}

}


