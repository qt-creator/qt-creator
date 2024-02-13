// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>

#include <functional>

namespace ProjectExplorer {

class ProjectUpdateInfo;
class ExtraCompiler;

class PROJECTEXPLORER_EXPORT ProjectUpdater
{
public:
    virtual ~ProjectUpdater() = default;

    virtual void update(const ProjectUpdateInfo &projectUpdateInfo,
                        const QList<ExtraCompiler *> &extraCompilers = {}) = 0;
    virtual void cancel() = 0;
};

class PROJECTEXPLORER_EXPORT ProjectUpdaterFactory
{
public:
    ProjectUpdaterFactory();
    ~ProjectUpdaterFactory();

    static ProjectUpdater *createProjectUpdater(Utils::Id language);
    static ProjectUpdater *createCppProjectUpdater(); // Convenience for C++.

protected:
    void setLanguage(Utils::Id language);
    void setCreator(const std::function<ProjectUpdater *()> &creator);

private:
    std::function<ProjectUpdater *()> m_creator;
    Utils::Id m_language;
};

} // namespace ProjectExplorer
