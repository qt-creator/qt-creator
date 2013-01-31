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

#include "formeditorwidget.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"

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
#include <lineeditaction.h>

#include <utils/fileutils.h>

namespace QmlDesigner {

FormEditorWidget::FormEditorWidget(FormEditorView *view)
    : QWidget(),
    m_formEditorView(view)
{
    setStyleSheet(QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/formeditorstylesheet.css")));

    QVBoxLayout *fillLayout = new QVBoxLayout(this);
    fillLayout->setMargin(0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    QList<QAction*> upperActions;

    m_toolActionGroup = new QActionGroup(this);

    m_transformToolAction = m_toolActionGroup->addAction(tr("Transform Tool (Press Key Q)"));
    m_transformToolAction->setShortcut(Qt::Key_Q);
    m_transformToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_transformToolAction->setCheckable(true);
    m_transformToolAction->setChecked(true);
    m_transformToolAction->setIcon(QPixmap(":/icon/tool/transform.png"));
    connect(m_transformToolAction.data(), SIGNAL(triggered(bool)), SLOT(changeTransformTool(bool)));


    QActionGroup *layoutActionGroup = new QActionGroup(this);
    layoutActionGroup->setExclusive(false);
    m_snappingAction = layoutActionGroup->addAction(tr("Snap to guides (E)"));
    m_snappingAction->setShortcut(Qt::Key_E);
    m_snappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingAction->setCheckable(true);
    m_snappingAction->setChecked(true);
    m_snappingAction->setIcon(QPixmap(":/icon/layout/snapping.png"));

    m_snappingAndAnchoringAction = layoutActionGroup->addAction(tr("Toogle Snapping And Anchoring (Press Key R)"));
    m_snappingAndAnchoringAction->setShortcut(Qt::Key_R);
    m_snappingAndAnchoringAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingAndAnchoringAction->setCheckable(true);
    m_snappingAndAnchoringAction->setChecked(false);
    m_snappingAndAnchoringAction->setEnabled(false);
    m_snappingAndAnchoringAction->setVisible(false);
    m_snappingAndAnchoringAction->setIcon(QPixmap(":/icon/layout/snapping_and_anchoring.png"));

    addActions(layoutActionGroup->actions());
    upperActions.append(layoutActionGroup->actions());

    QAction *separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    addAction(separatorAction);
    upperActions.append(separatorAction);

    m_showBoundingRectAction = new QAction(tr("Show bounding rectangles and stripes for empty items (Press Key A)"), this);
    m_showBoundingRectAction->setShortcut(Qt::Key_A);
    m_showBoundingRectAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_showBoundingRectAction->setCheckable(true);
    m_showBoundingRectAction->setChecked(true);
    m_showBoundingRectAction->setIcon(QPixmap(":/icon/layout/boundingrect.png"));

    addAction(m_showBoundingRectAction.data());
    upperActions.append(m_showBoundingRectAction.data());

    m_selectOnlyContentItemsAction = new QAction(tr("Only select items with content (S)"), this);
    m_selectOnlyContentItemsAction->setShortcut(Qt::Key_S);
    m_selectOnlyContentItemsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_selectOnlyContentItemsAction->setCheckable(true);
    m_selectOnlyContentItemsAction->setChecked(false);
    m_selectOnlyContentItemsAction->setIcon(QPixmap(":/icon/selection/selectonlycontentitems.png"));

    addAction(m_selectOnlyContentItemsAction.data());
    upperActions.append(m_selectOnlyContentItemsAction.data());

    m_rootWidthAction = new LineEditAction(tr("width"), this);
    connect(m_rootWidthAction.data(), SIGNAL(textChanged(QString)), this, SLOT(changeRootItemWidth(QString)));
    addAction(m_rootWidthAction.data());
    upperActions.append(m_rootWidthAction.data());

    m_rootHeightAction =  new LineEditAction(tr("height"), this);
    connect(m_rootHeightAction.data(), SIGNAL(textChanged(QString)), this, SLOT(changeRootItemHeight(QString)));
    addAction(m_rootHeightAction.data());
    upperActions.append(m_rootHeightAction.data());

    m_snappingAndAnchoringAction = layoutActionGroup->addAction(tr("Toogle Snapping And Anchoring (Press Key R)"));

    m_toolBox = new ToolBox(this);
    fillLayout->addWidget(m_toolBox.data());
    m_toolBox->setLeftSideActions(upperActions);

    m_graphicsView = new FormEditorGraphicsView(this);
    fillLayout->addWidget(m_graphicsView.data());

    m_graphicsView.data()->setStyleSheet(
            QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css")));

    QList<QAction*> lowerActions;

    m_zoomAction = new ZoomAction(m_toolActionGroup.data());
    connect(m_zoomAction.data(), SIGNAL(zoomLevelChanged(double)), SLOT(setZoomLevel(double)));
    addAction(m_zoomAction.data());
    upperActions.append(m_zoomAction.data());
    m_toolBox->addRightSideAction(m_zoomAction.data());

    m_resetAction = new QAction(tr("Reset view (R)"), this);
    m_resetAction->setShortcut(Qt::Key_R);
    m_resetAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_resetAction->setIcon(QPixmap(":/icon/reset.png"));
    connect(m_resetAction.data(), SIGNAL(triggered(bool)), this, SLOT(resetNodeInstanceView()));
    addAction(m_resetAction.data());
    upperActions.append(m_resetAction.data());
    m_toolBox->addRightSideAction(m_resetAction.data());
}

void FormEditorWidget::changeTransformTool(bool checked)
{
    if (checked)

        m_formEditorView->changeToTransformTools();
}

void FormEditorWidget::changeRootItemWidth(const QString &widthText)
{
    bool canConvert;
    int width = widthText.toInt(&canConvert);
    if (canConvert)
        m_formEditorView->rootModelNode().setAuxiliaryData("width", width);
    else
        m_formEditorView->rootModelNode().setAuxiliaryData("width", QVariant());
}

void FormEditorWidget::changeRootItemHeight(const QString &heighText)
{
    bool canConvert;
    int height = heighText.toInt(&canConvert);
    if (canConvert)
        m_formEditorView->rootModelNode().setAuxiliaryData("height", height);
    else
        m_formEditorView->rootModelNode().setAuxiliaryData("height", QVariant());
}

void FormEditorWidget::resetNodeInstanceView()
{
    m_formEditorView->setCurrentState(m_formEditorView->baseState());
    m_formEditorView->emitCustomNotification(QLatin1String("reset QmlPuppet"));
}

void FormEditorWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->delta() > 0)
            zoomAction()->zoomOut();
        else
            zoomAction()->zoomIn();

        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }

}

void FormEditorWidget::updateActions()
{
    if (m_formEditorView->model() && m_formEditorView->rootModelNode().isValid()) {
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("width") && m_formEditorView->rootModelNode().auxiliaryData("width").isValid())
            m_rootWidthAction->setLineEditText(m_formEditorView->rootModelNode().auxiliaryData("width").toString());
        else
            m_rootWidthAction->clearLineEditText();
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("height") && m_formEditorView->rootModelNode().auxiliaryData("height").isValid())
            m_rootHeightAction->setLineEditText(m_formEditorView->rootModelNode().auxiliaryData("height").toString());
        else
            m_rootHeightAction->clearLineEditText();
    } else {
        m_rootWidthAction->clearLineEditText();
        m_rootHeightAction->clearLineEditText();
    }
}


void FormEditorWidget::resetView()
{
    setRootItemRect(QRectF());
}

void FormEditorWidget::centerScene()
{
    m_graphicsView->centerOn(rootItemRect().center());
}

void FormEditorWidget::setFocus()
{
    m_graphicsView->setFocus(Qt::OtherFocusReason);
}

FormEditorCrumbleBar *FormEditorWidget::formEditorCrumbleBar() const
{
    return toolBox()->formEditorCrumbleBar();
}

ZoomAction *FormEditorWidget::zoomAction() const
{
    return m_zoomAction.data();
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

ToolBox *FormEditorWidget::toolBox() const
{
     return m_toolBox.data();
}

double FormEditorWidget::spacing() const
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.itemSpacing;
}

double FormEditorWidget::margins() const
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.snapMargin;
}


QString FormEditorWidget::contextHelpId() const
{
    if (!m_formEditorView)
        return QString();

    QList<ModelNode> nodes = m_formEditorView->selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("QtQuick", "QML");
    }

    return helpId;
}

void FormEditorWidget::setRootItemRect(const QRectF &rect)
{
    m_graphicsView->setRootItemRect(rect);
}

QRectF FormEditorWidget::rootItemRect() const
{
    return m_graphicsView->rootItemRect();
}

}


