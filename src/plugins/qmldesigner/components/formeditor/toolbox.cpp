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

#include "toolbox.h"
#include "utils/styledbar.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QPainter>
#include <QtDebug>
#include <QFile>
#include <QVariant>

namespace QmlDesigner {

ToolBox::ToolBox(QWidget *parentWidget)
    : Utils::StyledBar(parentWidget),
  m_leftToolBar(new QToolBar("LeftSidebar", this)),
  m_rightToolBar(new QToolBar("RightSidebar", this))
{
    QHBoxLayout *fillLayout = new QHBoxLayout(this);
    fillLayout->setMargin(0);
    fillLayout->setSpacing(0);

    m_leftToolBar->setFloatable(true);
    m_leftToolBar->setMovable(true);
    m_leftToolBar->setOrientation(Qt::Horizontal);
    m_leftToolBar->setIconSize(QSize(24, 24));

    QToolBar *stretchToolbar = new QToolBar(this);

    m_leftToolBar->setProperty("panelwidget", true);
    m_leftToolBar->setProperty("panelwidget_singlerow", true);

    m_rightToolBar->setProperty("panelwidget", true);
    m_rightToolBar->setProperty("panelwidget_singlerow", true);

    stretchToolbar->setProperty("panelwidget", true);
    stretchToolbar->setProperty("panelwidget_singlerow", true);

    stretchToolbar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_rightToolBar->setOrientation(Qt::Horizontal);
    m_rightToolBar->setIconSize(QSize(24, 24));
    fillLayout->addWidget(m_leftToolBar);
    fillLayout->addWidget(stretchToolbar);
    fillLayout->addWidget(m_rightToolBar);

    setLayout(fillLayout);
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
