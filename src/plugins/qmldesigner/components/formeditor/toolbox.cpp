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

#include <QToolBar>
#include <QHBoxLayout>
#include <QPainter>
#include <QtDebug>
#include <QFile>

namespace QmlDesigner {

ToolBox::ToolBox(QWidget *parentWidget)
  :  QWidget(parentWidget),
  m_toolBar(new QToolBar("Sidebar", this))
{
    QHBoxLayout *fillLayout = new QHBoxLayout(this);
    fillLayout->setMargin(0);
    setLayout(fillLayout);
    //setContentsMargins(4, 12, 4, 4);

    m_toolBar->setFloatable(true);
    m_toolBar->setMovable(true);
    m_toolBar->setOrientation(Qt::Horizontal);
    m_toolBar->setIconSize(QSize(24, 24));
    fillLayout->addWidget(m_toolBar);


//    QPalette toolPalette(palette());
//    QColor newColor(palette().color(QPalette::Window));
//    newColor.setAlphaF(0.7);
//    toolPalette.setColor(QPalette::Window, newColor);
//    setPalette(toolPalette);
//    setBackgroundRole(QPalette::Window);
//    setAutoFillBackground(true);
//    setAttribute(Qt::WA_NoSystemBackground, true);

}



void ToolBox::setActions(const QList<QAction*> &actions)
{
    m_toolBar->clear();
    m_toolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::addAction(QAction *action)
{
    m_toolBar->addAction(action);
}

} // namespace QmlDesigner
