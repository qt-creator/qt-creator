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
