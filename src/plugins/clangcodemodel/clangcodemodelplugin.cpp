// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangcodemodelplugin.h"

#include "clangconstants.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"

#ifdef WITH_TESTS
#  include "test/activationsequenceprocessortest.h"
#  include "test/clangbatchfileprocessor.h"
#  include "test/clangdtests.h"
#  include "test/clangfixittest.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/textmark.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/temporarydirectory.h>

using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

void ClangCodeModelPlugin::generateCompilationDB()
{
    using namespace CppEditor;

    ProjectExplorer::Target *target = ProjectExplorer::SessionManager::startupTarget();
    if (!target)
        return;

    const auto projectInfo = CppModelManager::instance()->projectInfo(target->project());
    if (!projectInfo)
        return;
    FilePath baseDir = projectInfo->buildRoot();
    if (baseDir == target->project()->projectDirectory())
        baseDir = TemporaryDirectory::masterDirectoryFilePath();

    QFuture<GenerateCompilationDbResult> task
            = Utils::runAsync(&Internal::generateCompilationDB, ProjectInfoList{projectInfo},
                              baseDir, CompilationDbPurpose::Project,
                              warningsConfigForProject(target->project()),
                              globalClangOptions(),
                              FilePath());
    Core::ProgressManager::addTask(task, tr("Generating Compilation DB"), "generate compilation db");
    m_generatorWatcher.setFuture(task);
}

ClangCodeModelPlugin::~ClangCodeModelPlugin()
{
    m_generatorWatcher.waitForFinished();
}

bool ClangCodeModelPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    ProjectExplorer::TaskHub::addCategory(Constants::TASK_CATEGORY_DIAGNOSTICS,
                                          tr("Clang Code Model"));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            &ProjectExplorer::ProjectExplorerPlugin::finishedInitialization,
            this,
            &ClangCodeModelPlugin::maybeHandleBatchFileAndExit);

    CppEditor::CppModelManager::instance()->activateClangCodeModel(
                std::make_unique<ClangModelManagerSupport>());

    createCompilationDBAction();

    return true;
}

void ClangCodeModelPlugin::createCompilationDBAction()
{
    // generate compile_commands.json
    m_generateCompilationDBAction = new ParameterAction(
                tr("Generate Compilation Database"),
                tr("Generate Compilation Database for \"%1\""),
                ParameterAction::AlwaysEnabled, this);
    ProjectExplorer::Project *startupProject = ProjectExplorer::SessionManager::startupProject();
    if (startupProject)
        m_generateCompilationDBAction->setParameter(startupProject->displayName());
    Core::Command *command = Core::ActionManager::registerAction(m_generateCompilationDBAction,
                                                                 Constants::GENERATE_COMPILATION_DB);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_generateCompilationDBAction->text());

    connect(&m_generatorWatcher, &QFutureWatcher<GenerateCompilationDbResult>::finished,
            this, [this] {
        const GenerateCompilationDbResult result = m_generatorWatcher.result();
        QString message;
        if (result.error.isEmpty()) {
            message = tr("Clang compilation database generated at \"%1\".")
                    .arg(QDir::toNativeSeparators(result.filePath));
        } else {
            message = tr("Generating Clang compilation database failed: %1").arg(result.error);
        }
        Core::MessageManager::writeFlashing(message);
        m_generateCompilationDBAction->setEnabled(true);
    });
    connect(m_generateCompilationDBAction, &QAction::triggered, this, [this] {
        if (!m_generateCompilationDBAction->isEnabled()) {
            Core::MessageManager::writeDisrupting("Cannot generate compilation database: "
                                                  "Generator is already running.");
            return;
        }
        ProjectExplorer::Project * const project
                = ProjectExplorer::SessionManager::startupProject();
        if (!project) {
            Core::MessageManager::writeDisrupting("Cannot generate compilation database: "
                                                  "No active project.");
            return;
        }
        const CppEditor::ProjectInfo::ConstPtr projectInfo = CppEditor::CppModelManager::instance()
                ->projectInfo(project);
        if (!projectInfo || projectInfo->projectParts().isEmpty()) {
            Core::MessageManager::writeDisrupting("Cannot generate compilation database: "
                                                  "Project has no C/C++ project parts.");
            return;
        }
        m_generateCompilationDBAction->setEnabled(false);
        generateCompilationDB();
    });
    connect(CppEditor::CppModelManager::instance(), &CppEditor::CppModelManager::projectPartsUpdated,
            this, [this](ProjectExplorer::Project *project) {
        if (project != ProjectExplorer::SessionManager::startupProject())
            return;
        m_generateCompilationDBAction->setParameter(project->displayName());
    });
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this,
            [this](ProjectExplorer::Project *project) {
        m_generateCompilationDBAction->setParameter(project ? project->displayName() : "");
    });
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::projectDisplayNameChanged,
            this,
            [this](ProjectExplorer::Project *project) {
        if (project != ProjectExplorer::SessionManager::startupProject())
            return;
        m_generateCompilationDBAction->setParameter(project->displayName());
    });
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::projectAdded, this,
            [this](ProjectExplorer::Project *project) {
        project->registerGenerator(Constants::GENERATE_COMPILATION_DB,
                                   m_generateCompilationDBAction->text(),
                                   [this] { m_generateCompilationDBAction->trigger(); });
    });
}

// For e.g. creation of profile-guided optimization builds.
void ClangCodeModelPlugin::maybeHandleBatchFileAndExit() const
{
#ifdef WITH_TESTS
    const QString batchFilePath = qtcEnvironmentVariable("QTC_CLANG_BATCH");
    if (!batchFilePath.isEmpty() && QTC_GUARD(QFileInfo::exists(batchFilePath))) {
        const bool runSucceeded = runClangBatchFile(batchFilePath);
        QCoreApplication::exit(!runSucceeded);
    }
#endif
}

#ifdef WITH_TESTS
QVector<QObject *> ClangCodeModelPlugin::createTestObjects() const
{
    return {
        new Tests::ActivationSequenceProcessorTest,
        new Tests::ClangdTestCompletion,
        new Tests::ClangdTestExternalChanges,
        new Tests::ClangdTestFindReferences,
        new Tests::ClangdTestFollowSymbol,
        new Tests::ClangdTestHighlighting,
        new Tests::ClangdTestLocalReferences,
        new Tests::ClangdTestTooltips,
        new Tests::ClangFixItTest,
    };
}
#endif

} // namespace Internal
} // namespace Clang
