// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangcodemodelplugin.h"

#include "clangcodemodeltr.h"
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

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangCodeModel::Internal {

void ClangCodeModelPlugin::generateCompilationDB()
{
    using namespace CppEditor;

    Target *target = SessionManager::startupTarget();
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
    ProgressManager::addTask(task, Tr::tr("Generating Compilation DB"), "generate compilation db");
    m_generatorWatcher.setFuture(task);
}

ClangCodeModelPlugin::~ClangCodeModelPlugin()
{
    m_generatorWatcher.waitForFinished();
}

void ClangCodeModelPlugin::initialize()
{
    TaskHub::addCategory(Constants::TASK_CATEGORY_DIAGNOSTICS, Tr::tr("Clang Code Model"));

    connect(ProjectExplorerPlugin::instance(),
            &ProjectExplorerPlugin::finishedInitialization,
            this,
            &ClangCodeModelPlugin::maybeHandleBatchFileAndExit);

    CppEditor::CppModelManager::instance()->activateClangCodeModel(
                std::make_unique<ClangModelManagerSupport>());

    createCompilationDBAction();

#ifdef WITH_TESTS
    addTest<Tests::ActivationSequenceProcessorTest>();
    addTest<Tests::ClangdTestCompletion>();
    addTest<Tests::ClangdTestExternalChanges>();
    addTest<Tests::ClangdTestFindReferences>();
    addTest<Tests::ClangdTestFollowSymbol>();
    addTest<Tests::ClangdTestHighlighting>();
    addTest<Tests::ClangdTestLocalReferences>();
    addTest<Tests::ClangdTestTooltips>();
    addTest<Tests::ClangFixItTest>();
#endif
}

void ClangCodeModelPlugin::createCompilationDBAction()
{
    // generate compile_commands.json
    m_generateCompilationDBAction = new ParameterAction(
                Tr::tr("Generate Compilation Database"),
                Tr::tr("Generate Compilation Database for \"%1\""),
                ParameterAction::AlwaysEnabled, this);
    Project *startupProject = SessionManager::startupProject();
    if (startupProject)
        m_generateCompilationDBAction->setParameter(startupProject->displayName());
    Command *command = ActionManager::registerAction(m_generateCompilationDBAction,
                                                     Constants::GENERATE_COMPILATION_DB);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_generateCompilationDBAction->text());

    connect(&m_generatorWatcher, &QFutureWatcher<GenerateCompilationDbResult>::finished,
            this, [this] {
        const GenerateCompilationDbResult result = m_generatorWatcher.result();
        QString message;
        if (result.error.isEmpty()) {
            message = Tr::tr("Clang compilation database generated at \"%1\".")
                    .arg(QDir::toNativeSeparators(result.filePath));
        } else {
            message = Tr::tr("Generating Clang compilation database failed: %1").arg(result.error);
        }
        MessageManager::writeFlashing(message);
        m_generateCompilationDBAction->setEnabled(true);
    });
    connect(m_generateCompilationDBAction, &QAction::triggered, this, [this] {
        if (!m_generateCompilationDBAction->isEnabled()) {
            MessageManager::writeDisrupting("Cannot generate compilation database: "
                                            "Generator is already running.");
            return;
        }
        Project * const project = SessionManager::startupProject();
        if (!project) {
            MessageManager::writeDisrupting("Cannot generate compilation database: "
                                            "No active project.");
            return;
        }
        const CppEditor::ProjectInfo::ConstPtr projectInfo = CppEditor::CppModelManager::instance()
                ->projectInfo(project);
        if (!projectInfo || projectInfo->projectParts().isEmpty()) {
            MessageManager::writeDisrupting("Cannot generate compilation database: "
                                            "Project has no C/C++ project parts.");
            return;
        }
        m_generateCompilationDBAction->setEnabled(false);
        generateCompilationDB();
    });
    connect(CppEditor::CppModelManager::instance(), &CppEditor::CppModelManager::projectPartsUpdated,
            this, [this](Project *project) {
        if (project != SessionManager::startupProject())
            return;
        m_generateCompilationDBAction->setParameter(project->displayName());
    });
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, [this](Project *project) {
        m_generateCompilationDBAction->setParameter(project ? project->displayName() : "");
    });
    connect(SessionManager::instance(), &SessionManager::projectDisplayNameChanged,
            this, [this](Project *project) {
        if (project != SessionManager::startupProject())
            return;
        m_generateCompilationDBAction->setParameter(project->displayName());
    });
    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, [this](Project *project) {
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

} // ClangCodeModel::Internal
