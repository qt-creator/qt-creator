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

#include "projectpanelfactory.h"

namespace ProjectExplorer {

static QList<ProjectPanelFactory *> s_factories;

ProjectPanelFactory::ProjectPanelFactory()
    : m_priority(0),
      m_supportsFunction([] (Project *) { return true; })
{}

int ProjectPanelFactory::priority() const
{
    return m_priority;
}

void ProjectPanelFactory::setPriority(int priority)
{
    m_priority = priority;
}

QString ProjectPanelFactory::displayName() const
{
    return m_displayName;
}

void ProjectPanelFactory::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void ProjectPanelFactory::registerFactory(ProjectPanelFactory *factory)
{
    auto it = std::lower_bound(s_factories.begin(), s_factories.end(), factory,
        [](ProjectPanelFactory *a, ProjectPanelFactory *b)  {
            return (a->priority() == b->priority() && a < b) || a->priority() < b->priority();
        });

    s_factories.insert(it, factory);
}

QList<ProjectPanelFactory *> ProjectPanelFactory::factories()
{
    return s_factories;
}

void ProjectPanelFactory::destroyFactories()
{
    qDeleteAll(s_factories);
    s_factories.clear();
}

bool ProjectPanelFactory::supports(Project *project)
{
    return m_supportsFunction(project);
}

void ProjectPanelFactory::setSupportsFunction(std::function<bool (Project *)> function)
{
    m_supportsFunction = function;
}

QWidget *ProjectPanelFactory::createWidget(Project *project)
{
    return m_createWidgetFunction(project);
}

} // namespace ProjectExplorer
