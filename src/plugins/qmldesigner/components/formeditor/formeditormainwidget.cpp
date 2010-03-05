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

#include "formeditormainwidget.h"

#include <QAction>
#include <formeditormainview.h>
#include <QtDebug>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFile>
#include "toolbox.h"
#include "componentaction.h"
#include "zoomaction.h"
#include <QWheelEvent>
#include <cmath>

namespace QmlDesigner {

FormEditorMainWidget::FormEditorMainWidget(FormEditorMainView *mainView)
  : QWidget(),
    m_formEditorMainView(mainView),
    m_stackedWidget(new QStackedWidget(this))
{
    QFile file(":/qmldesigner/stylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QVBoxLayout *fillLayout = new QVBoxLayout(this);
    fillLayout->setMargin(0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    QActionGroup *toolActionGroup = new QActionGroup(this);

    QAction *transformToolAction = toolActionGroup->addAction("Transform Tool (Press Key Q)");
    transformToolAction->setShortcut(Qt::Key_Q);
    transformToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    transformToolAction->setCheckable(true);
    transformToolAction->setChecked(true);
    transformToolAction->setIcon(QPixmap(":/icon/tool/transform.png"));
    connect(transformToolAction, SIGNAL(triggered(bool)), SLOT(changeTransformTool(bool)));

    QAction *anchorToolAction = toolActionGroup->addAction("Anchor Tool (Press Key W)");
    anchorToolAction->setShortcut(Qt::Key_W);
    anchorToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    anchorToolAction->setCheckable(true);
    anchorToolAction->setIcon(QPixmap(":/icon/tool/anchor.png"));
    connect(anchorToolAction, SIGNAL(triggered(bool)), SLOT(changeAnchorTool(bool)));

    addActions(toolActionGroup->actions());


    m_componentAction = new ComponentAction(toolActionGroup);
    addAction(m_componentAction.data());

    m_zoomAction = new ZoomAction(toolActionGroup);
    addAction(m_zoomAction.data());

    ToolBox *toolBox = new ToolBox(this);
    toolBox->setActions(actions());
    fillLayout->addWidget(toolBox);

    fillLayout->addWidget(m_stackedWidget.data());
}

void FormEditorMainWidget::addWidget(QWidget *widget)
{
    m_stackedWidget->addWidget(widget);
}

QWidget *FormEditorMainWidget::currentWidget() const
{
    return m_stackedWidget->currentWidget();
}

void FormEditorMainWidget::setCurrentWidget(QWidget *widget)
{
    m_stackedWidget->setCurrentWidget(widget);
}

void FormEditorMainWidget::setModel(Model *model)
{
    m_componentAction->setDisabled(true);

    if (model) {
        m_componentAction->setModel(model->masterModel());
        m_componentAction->setEnabled(true);
    }
}

void FormEditorMainWidget::changeTransformTool(bool checked)
{
    if (checked)
        m_formEditorMainView->changeToTransformTools();
}

void FormEditorMainWidget::changeAnchorTool(bool checked)
{
    if (checked)
        m_formEditorMainView->changeToAnchorTool();
}

ComponentAction *FormEditorMainWidget::componentAction() const
{
    return m_componentAction.data();
}

ZoomAction *FormEditorMainWidget::zoomAction() const
{
    return m_zoomAction.data();
}

void FormEditorMainWidget::wheelEvent(QWheelEvent *event)
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

}
