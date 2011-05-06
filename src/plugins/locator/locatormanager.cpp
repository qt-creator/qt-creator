/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "locatormanager.h"
#include "locatorwidget.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

using namespace Locator;
using namespace Locator::Internal;

LocatorManager *LocatorManager::m_instance = 0;

LocatorManager::LocatorManager(LocatorWidget *locatorWidget)
  : QObject(locatorWidget),
    m_locatorWidget(locatorWidget)
{
    m_instance = this;
}

LocatorManager::~LocatorManager()
{
    ExtensionSystem::PluginManager::instance()->removeObject(this);
    m_instance = 0;
}

void LocatorManager::show(const QString &text,
                            int selectionStart, int selectionLength)
{
    QTC_ASSERT(m_locatorWidget, return);
    m_locatorWidget->show(text, selectionStart, selectionLength);
}
