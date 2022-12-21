// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppprojectupdaterinterface.h"
#include "projectinfo.h"

#include <projectexplorer/extracompiler.h>
#include <utils/futuresynchronizer.h>

#include <QFutureWatcher>

namespace CppEditor {
class ProjectInfo;

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
    void onProjectInfoGenerated();
    void checkForExtraCompilersFinished();

private:
    ProjectExplorer::ProjectUpdateInfo m_projectUpdateInfo;
    QList<QPointer<ProjectExplorer::ExtraCompiler>> m_extraCompilers;

    QFutureWatcher<ProjectInfo::ConstPtr> m_generateFutureWatcher;
    bool m_isProjectInfoGenerated = false;
    QSet<QFutureWatcher<void> *> m_extraCompilersFutureWatchers;
    std::unique_ptr<QFutureInterface<void>> m_projectUpdateFutureInterface;
    Utils::FutureSynchronizer m_futureSynchronizer;
};

} // namespace CppEditor
