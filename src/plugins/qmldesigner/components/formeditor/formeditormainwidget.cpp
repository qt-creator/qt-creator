/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    QFile file(":/qmldesigner/formeditorstylesheet.css");
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
