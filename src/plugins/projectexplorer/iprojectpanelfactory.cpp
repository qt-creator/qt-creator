/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iprojectpanelfactory.h"

using namespace ProjectExplorer;

QList<IProjectPanelFactory *> IProjectPanelFactory::s_factories;

IProjectPanelFactory::IProjectPanelFactory()
    : m_priority(0),
      m_supportsFunction(&supportsAllProjects)
{}

int IProjectPanelFactory::priority() const
{
    return m_priority;
}

void IProjectPanelFactory::setPriority(int priority)
{
    m_priority = priority;
}

QString IProjectPanelFactory::displayName() const
{
    return m_displayName;
}

void IProjectPanelFactory::setDisplayName(const QString &name)
{
    m_displayName = name;
}

bool IProjectPanelFactory::prioritySort(IProjectPanelFactory *a, IProjectPanelFactory *b)
{
    return (a->priority() == b->priority() && a < b)
            || a->priority() < b->priority();
}

bool IProjectPanelFactory::supportsAllProjects(Project *)
{
    return true;
}

void IProjectPanelFactory::registerFactory(IProjectPanelFactory *factory)
{
    auto it = std::lower_bound(s_factories.begin(), s_factories.end(), factory, &IProjectPanelFactory::prioritySort);
    s_factories.insert(it, factory);
}

QList<IProjectPanelFactory *> IProjectPanelFactory::factories()
{
    return s_factories;
}

bool IProjectPanelFactory::supports(Project *project)
{
    return m_supportsFunction(project);
}

void IProjectPanelFactory::setSupportsFunction(std::function<bool (Project *)> function)
{
    m_supportsFunction = function;
}

QWidget *IProjectPanelFactory::createWidget(Project *project)
{
    return m_createWidgetFunction(project);
}
