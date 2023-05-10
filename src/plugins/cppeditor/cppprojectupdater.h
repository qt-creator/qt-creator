// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppprojectupdaterinterface.h"

#include <utils/futuresynchronizer.h>

namespace ProjectExplorer { class ExtraCompiler; }
namespace Tasking { class TaskTree; }

namespace CppEditor {

namespace Internal {

// registered in extensionsystem's object pool for plugins with weak dependency to CppEditor
class CppProjectUpdaterFactory : public QObject
{
    Q_OBJECT
public:
    CppProjectUpdaterFactory();

    // keep the namespace, for the type name in the invokeMethod call
    Q_INVOKABLE CppEditor::CppProjectUpdaterInterface *create();
};

} // namespace Internal

class CPPEDITOR_EXPORT CppProjectUpdater final : public QObject, public CppProjectUpdaterInterface
{
    Q_OBJECT

public:
    CppProjectUpdater();
    ~CppProjectUpdater() override;

    void update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo) override;
    void update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo,
                const QList<ProjectExplorer::ExtraCompiler *> &extraCompilers);
    void cancel() override;

private:
    Utils::FutureSynchronizer m_futureSynchronizer;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
};

} // namespace CppEditor
