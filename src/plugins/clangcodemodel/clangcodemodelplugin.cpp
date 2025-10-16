// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangcodemodeltr.h"
#include "clangconstants.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"

#ifdef WITH_TESTS
#  include "test/activationsequenceprocessortest.h"
#  include "test/clangdtests.h"
#  include "test/clangfixittest.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <solutions/tasking/tasktreerunner.h>

#include <texteditor/textmark.h>

#include <utils/action.h>
#include <utils/async.h>
#include <utils/temporarydirectory.h>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace ClangCodeModel::Internal {

class ClangCodeModelPlugin final: public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangCodeModel.json")

public:
    void initialize() final;

private:
    void updateGeneratorName(Project *project);
    void generateCompilationDB();
    void createCompilationDBAction();

    Utils::Action *m_generateCompilationDBAction = nullptr;
    Task m_generateCompilationDBError;
    SingleTaskTreeRunner m_taskTreeRunner;
};

void ClangCodeModelPlugin::initialize()
{
    TaskHub::addCategory({Constants::TASK_CATEGORY_DIAGNOSTICS,
                          Tr::tr("Clang Code Model"),
                          Tr::tr("C++ code issues that Clangd found in the current document.")});
    CppModelManager::activateClangCodeModel(std::make_unique<ClangModelManagerSupport>());
    createCompilationDBAction();

    ActionBuilder updateStaleIndexEntries(this, "ClangCodeModel.UpdateStaleIndexEntries");
    updateStaleIndexEntries.setText(Tr::tr("Update Potentially Stale Clangd Index Entries"));
    updateStaleIndexEntries.addOnTriggered(this, &ClangModelManagerSupport::updateStaleIndexEntries);
    updateStaleIndexEntries.addToContainer(CppEditor::Constants::M_TOOLS_CPP);
    updateStaleIndexEntries.addToContainer(CppEditor::Constants::M_CONTEXT);

#ifdef WITH_TESTS
    addTestCreator(createActivationSequenceProcessorTest);
    addTestCreator(createClangdTestCompletion);
    addTestCreator(createClangdTestExternalChanges);
    addTestCreator(createClangdTestFindReferences);
    addTestCreator(createClangdTestFollowSymbol);
    addTestCreator(createClangdTestHighlighting);
    addTestCreator(createClangdTestIndirectChanges);
    addTestCreator(createClangdTestLocalReferences);
    addTestCreator(createClangdTestTooltips);
    addTestCreator(createClangFixItTest);
#endif
}

void ClangCodeModelPlugin::updateGeneratorName(Project *project)
{
    m_generateCompilationDBAction->setParameter(project ? project->displayName() : QString());
    if (project) {
        project->registerGenerator(Constants::GENERATE_COMPILATION_DB,
                                   m_generateCompilationDBAction->text(),
                                   [this] { m_generateCompilationDBAction->trigger(); });
    }
}

void ClangCodeModelPlugin::generateCompilationDB()
{
    Project *project = ProjectManager::startupProject();
    if (!project || !project->activeKit())
        return;

    const auto projectInfo = CppModelManager::projectInfo(project);
    if (!projectInfo)
        return;
    FilePath baseDir = projectInfo->buildRoot();
    if (baseDir == project->projectDirectory())
        baseDir = TemporaryDirectory::masterDirectoryFilePath();

    const auto onSetup = [projectInfo, baseDir, project](
                             Async<GenerateCompilationDbResult> &task) {
        task.setConcurrentCallData(&Internal::generateCompilationDB,
                                   ProjectInfoList{projectInfo}, baseDir,
                                   CompilationDbPurpose::Project,
                                   warningsConfigForProject(project),
                                   globalClangOptions(), FilePath());
    };
    const auto onDone = [this](const Async<GenerateCompilationDbResult> &task) {
        QString message;
        if (task.isResultAvailable()) {
            const GenerateCompilationDbResult result = task.result();
            if (result) {
                message = Tr::tr("Clang compilation database generated at \"%1\".")
                              .arg(result->toUserOutput());
            } else {
                message = Tr::tr("Generating Clang compilation database failed: %1")
                              .arg(result.error());
            }
        } else {
            message = Tr::tr("Generating Clang compilation database canceled.");
        }
        MessageManager::writeFlashing(message);
        m_generateCompilationDBAction->setEnabled(true);
    };

    const auto onTreeSetup = [](TaskTree &taskTree) {
        auto progress = new TaskProgress(&taskTree);
        progress->setDisplayName(Tr::tr("Generating Compilation DB"));
        progress->setId("generate compilation db");
    };
    m_taskTreeRunner.start(
        {AsyncTask<GenerateCompilationDbResult>(onSetup, onDone)}, onTreeSetup);
}

void ClangCodeModelPlugin::createCompilationDBAction()
{
    // generate compile_commands.json
    ActionBuilder(this, Constants::GENERATE_COMPILATION_DB)
        .setParameterText(
            Tr::tr("Compilation Database for \"%1\""),
            Tr::tr("Compilation Database"),
            ActionBuilder::AlwaysEnabled)
        .bindContextAction(&m_generateCompilationDBAction)
        .setCommandAttribute(Command::CA_UpdateText)
        .setCommandDescription(Tr::tr("Generate Compilation Database"));

    updateGeneratorName(ProjectManager::startupProject());

    connect(m_generateCompilationDBAction, &QAction::triggered, this, [this] {
        TaskHub::clearAndRemoveTask(m_generateCompilationDBError);
        const auto setError = [this](const QString &reason) {
            m_generateCompilationDBError = OtherTask(
                Task::DisruptingError,
                Tr::tr("Cannot generate compilation database.").append('\n').append(reason));
            TaskHub::addTask(m_generateCompilationDBError);
        };
        if (!m_generateCompilationDBAction->isEnabled())
            return setError(Tr::tr("Generator is already running."));

        Project * const project = ProjectManager::startupProject();
        if (!project)
            return setError(Tr::tr("No active project."));
        const ProjectInfo::ConstPtr projectInfo = CppModelManager::projectInfo(project);
        if (!projectInfo || projectInfo->projectParts().isEmpty())
            return setError(Tr::tr("Project has no C/C++ project parts."));
        m_generateCompilationDBAction->setEnabled(false);
        generateCompilationDB();
    });
    connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated,
            this, [this](Project *project) {
        if (project != ProjectManager::startupProject())
            return;
        updateGeneratorName(project); // TODO: What does this have to do with project parts?
    });
    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            this, &ClangCodeModelPlugin::updateGeneratorName);
    connect(ProjectManager::instance(), &ProjectManager::projectDisplayNameChanged,
            this, [this](Project *project) {
        if (project != ProjectManager::startupProject())
            return;
        updateGeneratorName(project);
    });
}

} // namespace ClangCodeModel::Internal

#include "clangcodemodelplugin.moc"
