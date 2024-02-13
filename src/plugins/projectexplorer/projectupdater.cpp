// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectupdater.h"

#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/predicates.h>
#include <utils/qtcassert.h>

#include <QList>

using namespace Utils;

namespace ProjectExplorer {

static QList<ProjectUpdaterFactory *> &projectUpdaterFactories()
{
    static QList<ProjectUpdaterFactory *> theProjectUpdaterFactories;
    return theProjectUpdaterFactories;
}

ProjectUpdaterFactory::ProjectUpdaterFactory()
{
    projectUpdaterFactories().append(this);
}

ProjectUpdaterFactory::~ProjectUpdaterFactory()
{
    projectUpdaterFactories().removeOne(this);
}

void ProjectUpdaterFactory::setLanguage(Id language)
{
    m_language = language;
}

void ProjectUpdaterFactory::setCreator(const std::function<ProjectUpdater *()> &creator)
{
    m_creator = creator;
}

ProjectUpdater *ProjectUpdaterFactory::createProjectUpdater(Id language)
{
    const QList<ProjectUpdaterFactory *> &factories = projectUpdaterFactories();
    ProjectUpdaterFactory *factory =
        findOrDefault(factories, equal(&ProjectUpdaterFactory::m_language, language));
    QTC_ASSERT(factory, return nullptr);
    return factory->m_creator();
}

ProjectUpdater *ProjectUpdaterFactory::createCppProjectUpdater()
{
    return createProjectUpdater(Constants::CXX_LANGUAGE_ID);
}

} // ProjectExplorer
