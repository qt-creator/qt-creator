// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectpanelfactory.h"

#include "project.h"
#include "projectsettingswidget.h"
#include "projectwindow.h"

#include <utils/layoutbuilder.h>

using namespace ProjectExplorer::Internal;
using namespace Utils;

namespace ProjectExplorer {

static QList<ProjectPanelFactory *> s_factories;
bool s_sorted = false;

ProjectPanelFactory::ProjectPanelFactory()
    : m_supportsFunction([] (Project *) { return true; })
{
    s_factories.append(this);
    s_sorted = false;
}

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

QList<ProjectPanelFactory *> ProjectPanelFactory::factories()
{
    if (!s_sorted) {
        s_sorted = true;
        std::sort(s_factories.begin(), s_factories.end(),
                  [](ProjectPanelFactory *a, ProjectPanelFactory *b)  {
            return (a->priority() == b->priority() && a < b) || a->priority() < b->priority();
        });
    }
    return s_factories;
}

Id ProjectPanelFactory::id() const
{
    return m_id;
}

void ProjectPanelFactory::setId(Id id)
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
