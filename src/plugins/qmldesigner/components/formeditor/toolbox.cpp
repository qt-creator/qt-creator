/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "toolbox.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QFrame>

namespace QmlDesigner {

ToolBox::ToolBox(QWidget *parentWidget)
    : Utils::StyledBar(parentWidget),
  m_leftToolBar(new QToolBar(QLatin1String("LeftSidebar"), this)),
  m_rightToolBar(new QToolBar(QLatin1String("RightSidebar"), this))
{
    m_leftToolBar->setFloatable(true);
    m_leftToolBar->setMovable(true);
    m_leftToolBar->setOrientation(Qt::Horizontal);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setMargin(0);
    horizontalLayout->setSpacing(0);

    QToolBar *stretchToolbar = new QToolBar(this);

    m_leftToolBar->setProperty("panelwidget", true);
    m_leftToolBar->setProperty("panelwidget_singlerow", false);

    m_rightToolBar->setProperty("panelwidget", true);
    m_rightToolBar->setProperty("panelwidget_singlerow", false);

    stretchToolbar->setProperty("panelwidget", true);
    stretchToolbar->setProperty("panelwidget_singlerow", false);

    stretchToolbar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_rightToolBar->setOrientation(Qt::Horizontal);
    horizontalLayout->addWidget(m_leftToolBar);
    horizontalLayout->addWidget(stretchToolbar);
    horizontalLayout->addWidget(m_rightToolBar);
}

void ToolBox::setLeftSideActions(const QList<QAction*> &actions)
{
    m_leftToolBar->clear();
    m_leftToolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::setRightSideActions(const QList<QAction*> &actions)
{
    m_rightToolBar->clear();
    m_rightToolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::addLeftSideAction(QAction *action)
{
    m_leftToolBar->addAction(action);
}

void ToolBox::addRightSideAction(QAction *action)
{
    m_rightToolBar->addAction(action);
}

QList<QAction*> ToolBox::actions() const
{
    return m_leftToolBar->actions() + m_rightToolBar->actions();
}

} // namespace QmlDesigner
