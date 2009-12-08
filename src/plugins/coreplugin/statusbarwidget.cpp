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

#include "statusbarwidget.h"

#include <QtGui/QWidget>

using namespace Core;

StatusBarWidget::StatusBarWidget(QObject *parent)
        : IContext(parent),
    m_widget(0),
    m_context(QList<int>()),
    m_defaultPosition(StatusBarWidget::First)
{
}

StatusBarWidget::~StatusBarWidget()
{
    delete m_widget;
}

QList<int> StatusBarWidget::context() const
{
    return m_context;
}

QWidget *StatusBarWidget::widget()
{
    return m_widget;
}

StatusBarWidget::StatusBarPosition StatusBarWidget::position() const
{
    return m_defaultPosition;
}

QWidget *StatusBarWidget::setWidget(QWidget *widget)
{
    QWidget *oldWidget = m_widget;
    m_widget = widget;
    return oldWidget;
}

void StatusBarWidget::setContext(const QList<int> &context)
{
    m_context = context;
}

void StatusBarWidget::setPosition(StatusBarWidget::StatusBarPosition position)
{
    m_defaultPosition = position;
}

