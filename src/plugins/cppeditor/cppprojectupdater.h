// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <projectexplorer/projectupdater.h>
#include <projectexplorer/rawprojectpart.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/futuresynchronizer.h>

namespace ProjectExplorer { class ExtraCompiler; }

namespace CppEditor {

namespace Internal {

// registered in extensionsystem's object pool for plugins with weak dependency to CppEditor
class CppProjectUpdaterFactory final
    : public ProjectExplorer::ProjectUpdaterFactory
{
public:
    CppProjectUpdaterFactory();
};

} // namespace Internal

class CPPEDITOR_EXPORT CppProjectUpdater final
    : public QObject, public ProjectExplorer::ProjectUpdater
{
    Q_OBJECT

public:
    void update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo,
                const QList<ProjectExplorer::ExtraCompiler *> &extraCompilers) override;
    void cancel() override;

private:
    Utils::FutureSynchronizer m_futureSynchronizer;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // namespace CppEditor
