/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "findplaceholder.h"

#include <extensionsystem/pluginmanager.h>

#include <QVBoxLayout>


using namespace Core;

FindToolBarPlaceHolder *FindToolBarPlaceHolder::m_current = 0;

FindToolBarPlaceHolder::FindToolBarPlaceHolder(QWidget *owner, QWidget *parent)
    : QWidget(parent), m_owner(owner), m_subWidget(0)
{
    setLayout(new QVBoxLayout);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    layout()->setMargin(0);
    ExtensionSystem::PluginManager::addObject(this);
}

FindToolBarPlaceHolder::~FindToolBarPlaceHolder()
{
    ExtensionSystem::PluginManager::removeObject(this);
    if (m_subWidget) {
        m_subWidget->setVisible(false);
        m_subWidget->setParent(0);
    }
    if (m_current == this)
        m_current = 0;
}

QWidget *FindToolBarPlaceHolder::owner() const
{
    return m_owner;
}

void FindToolBarPlaceHolder::setWidget(QWidget *widget)
{
    if (m_subWidget) {
        m_subWidget->setVisible(false);
        m_subWidget->setParent(0);
    }
    m_subWidget = widget;
    if (m_subWidget)
        layout()->addWidget(m_subWidget);
}

FindToolBarPlaceHolder *FindToolBarPlaceHolder::getCurrent()
{
    return m_current;
}

void FindToolBarPlaceHolder::setCurrent(FindToolBarPlaceHolder *placeHolder)
{
    m_current = placeHolder;
}
