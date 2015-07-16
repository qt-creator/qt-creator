/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "toolbox.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QDebug>
#include <QFrame>

namespace QmlDesigner {

ToolBox::ToolBox(QWidget *parentWidget)
    : Utils::StyledBar(parentWidget),
  m_leftToolBar(new QToolBar(QLatin1String("LeftSidebar"), this)),
  m_rightToolBar(new QToolBar(QLatin1String("RightSidebar"), this))
{
    setMaximumHeight(22);
    m_leftToolBar->setFloatable(true);
    m_leftToolBar->setMovable(true);
    m_leftToolBar->setOrientation(Qt::Horizontal);
    m_leftToolBar->setIconSize(QSize(24, 24));

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
    m_rightToolBar->setIconSize(QSize(24, 24));
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
    return QList<QAction*>() << m_leftToolBar->actions() << m_rightToolBar->actions();
}

} // namespace QmlDesigner
