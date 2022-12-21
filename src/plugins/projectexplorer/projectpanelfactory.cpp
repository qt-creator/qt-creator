// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectpanelfactory.h"

#include "project.h"
#include "projectwindow.h"

#include <utils/layoutbuilder.h>

using namespace ProjectExplorer::Internal;
using namespace Utils;

namespace ProjectExplorer {

static QList<ProjectPanelFactory *> s_factories;

ProjectPanelFactory::ProjectPanelFactory()
    : m_supportsFunction([] (Project *) { return true; })
{ }

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

Utils::Id ProjectPanelFactory::id() const
{
    return m_id;
}

void ProjectPanelFactory::setId(Utils::Id id)
{
    m_id = id;
}

ProjectSettingsWidget *ProjectPanelFactory::createWidget(Project *project) const
{
    return m_widgetCreator(project);
}

void ProjectPanelFactory::setCreateWidgetFunction(const WidgetCreator &createWidgetFunction)
{
    m_widgetCreator = createWidgetFunction;
}

bool ProjectPanelFactory::supports(Project *project)
{
    return m_supportsFunction(project);
}

void ProjectPanelFactory::setSupportsFunction(std::function<bool (Project *)> function)
{
    m_supportsFunction = function;
}

} // namespace ProjectExplorer
