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

#include "baseview.h"

#include <QtGui/QWidget>

using namespace Core;

BaseView::BaseView(QObject *parent)
        : IView(parent),
    m_viewName(""),
    m_widget(0),
    m_context(QList<int>()),
    m_defaultPosition(IView::First)
{
}

BaseView::~BaseView()
{
    delete m_widget;
}

QList<int> BaseView::context() const
{
    return m_context;
}

QWidget *BaseView::widget()
{
    return m_widget;
}

const char *BaseView::uniqueViewName() const
{
    return m_viewName;
}


IView::ViewPosition BaseView::defaultPosition() const
{
    return m_defaultPosition;
}

void BaseView::setUniqueViewName(const char *name)
{
    m_viewName = name;
}

QWidget *BaseView::setWidget(QWidget *widget)
{
    QWidget *oldWidget = m_widget;
    m_widget = widget;
    return oldWidget;
}

void BaseView::setContext(const QList<int> &context)
{
    m_context = context;
}

void BaseView::setDefaultPosition(IView::ViewPosition position)
{
    m_defaultPosition = position;
}

