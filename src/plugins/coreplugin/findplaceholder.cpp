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

#include "findplaceholder.h"
#include "find/findtoolbar.h"

#include <extensionsystem/pluginmanager.h>

#include <QVBoxLayout>

using namespace Core;

FindToolBarPlaceHolder *FindToolBarPlaceHolder::m_current = 0;

FindToolBarPlaceHolder::FindToolBarPlaceHolder(QWidget *owner, QWidget *parent)
    : QWidget(parent), m_owner(owner), m_subWidget(0), m_lightColored(false)
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

/*!
 * Returns if \a widget is a subwidget of the place holder's owner
 */
bool FindToolBarPlaceHolder::isUsedByWidget(QWidget *widget)
{
    QWidget *current = widget;
    while (current) {
        if (current == m_owner)
            return true;
        current = current->parentWidget();
    }
    return false;
}

void FindToolBarPlaceHolder::setWidget(Internal::FindToolBar *widget)
{
    if (m_subWidget) {
        m_subWidget->setVisible(false);
        m_subWidget->setParent(0);
    }
    m_subWidget = widget;
    if (m_subWidget) {
        m_subWidget->setLightColored(m_lightColored);
        m_subWidget->setLightColoredIcon(m_lightColored);
        layout()->addWidget(m_subWidget);
    }
}

FindToolBarPlaceHolder *FindToolBarPlaceHolder::getCurrent()
{
    return m_current;
}

void FindToolBarPlaceHolder::setCurrent(FindToolBarPlaceHolder *placeHolder)
{
    m_current = placeHolder;
}

void FindToolBarPlaceHolder::setLightColored(bool lightColored)
{
    m_lightColored = lightColored;
}

bool FindToolBarPlaceHolder::isLightColored() const
{
    return m_lightColored;
}
