/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangcodemodelplugin.h"

#include "clangconstants.h"
#include "clangprojectsettingswidget.h"
#include "clangutils.h"

#ifdef WITH_TESTS
#  include "test/clangbatchfileprocessor.h"
#  include "test/clangcodecompletion_test.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/textmark.h>

#include <QtConcurrent>

namespace ClangCodeModel {
namespace Internal {

static void addProjectPanelWidget()
{
    auto panelFactory = new ProjectExplorer::ProjectPanelFactory();
    panelFactory->setPriority(60);
    panelFactory->setDisplayName(ClangProjectSettingsWidget::tr("Clang Code Model"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ClangProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
}

void ClangCodeModelPlugin::generateCompilationDB() {
    using namespace CppTools;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || !project->activeTarget())
        return;

    m_generatorWatcher.setFuture(
        QtConcurrent::run(&Utils::generateCompilationDB,
                          CppModelManager::instance()->projectInfo(project)));
}

static bool isDBGenerationEnabled(ProjectExplorer::Project *project)
{
    using namespace CppTools;
    if (!project)
        return false;
    ProjectInfo projectInfo = CppModelManager::instance()->projectInfo(project);
    return projectInfo.isValid() && !projectInfo.projectParts().isEmpty();
}

ClangCodeModelPlugin::~ClangCodeModelPlugin()
{
    m_generatorWatcher.waitForFinished();
}

bool ClangCodeModelPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    ProjectExplorer::TaskHub::addCategory(Constants::TASK_CATEGORY_DIAGNOSTICS,
                                          tr("Clang Code Model"));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            &ProjectExplorer::ProjectExplorerPlugin::finishedInitialization,
            this,
            &ClangCodeModelPlugin::maybeHandleBatchFileAndExit);

    CppTools::CppModelManager::instance()->activateClangCodeModel(&m_modelManagerSupportProvider);

    addProjectPanelWidget();

    createCompilationDBButton();

    return true;
}

void ClangCodeModelPlugin::createCompilationDBButton()
{
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    // generate compile_commands.json
    m_generateCompilationDBAction = new ::Utils::ParameterAction(
                tr("Generate Compilation Database"),
                tr("Generate Compilation Database for \"%1\""),
                ::Utils::ParameterAction::AlwaysEnabled, this);

    ProjectExplorer::Project *startupProject = ProjectExplorer::SessionManager::startupProject();
    m_generateCompilationDBAction->setEnabled(isDBGenerationEnabled(startupProject));
    if (startupProject)
        m_generateCompilationDBAction->setParameter(startupProject->displayName());

    Core::Command *command = Core::ActionManager::registerAction(m_generateCompilationDBAction,
                                                                 Constants::GENERATE_COMPILATION_DB);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_generateCompilationDBAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);

    connect(&m_generatorWatcher, &QFutureWatcher<void>::finished, this, [this] () {
        m_generateCompilationDBAction->setEnabled(
                    isDBGenerationEnabled(ProjectExplorer::SessionManager::startupProject()));
    });
    connect(m_generateCompilationDBAction, &QAction::triggered, this, [this] {
        if (!m_generateCompilationDBAction->isEnabled())
            return;

        m_generateCompilationDBAction->setEnabled(false);
        generateCompilationDB();
    });
    connect(CppTools::CppModelManager::instance(), &CppTools::CppModelManager::projectPartsUpdated,
            this, [this](ProjectExplorer::Project *project) {
        if (project != ProjectExplorer::SessionManager::startupProject())
            return;
        m_generateCompilationDBAction->setParameter(project->displayName());
        if (!m_generatorWatcher.isRunning())
            m_generateCompilationDBAction->setEnabled(isDBGenerationEnabled(project));
    });
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this,
            [this](ProjectExplorer::Project *project) {
        m_generateCompilationDBAction->setParameter(project ? project->displayName() : "");
        if (!m_generatorWatcher.isRunning())
            m_generateCompilationDBAction->setEnabled(isDBGenerationEnabled(project));
    });
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::projectDisplayNameChanged,
            this,
            [this](ProjectExplorer::Project *project) {
        if (project != ProjectExplorer::SessionManager::startupProject())
            return;
        m_generateCompilationDBAction->setParameter(project->displayName());
    });
}

void ClangCodeModelPlugin::extensionsInitialized()
{
}

// For e.g. creation of profile-guided optimization builds.
void ClangCodeModelPlugin::maybeHandleBatchFileAndExit() const
{
#ifdef WITH_TESTS
    const QString batchFilePath = QString::fromLocal8Bit(qgetenv("QTC_CLANG_BATCH"));
    if (!batchFilePath.isEmpty() && QTC_GUARD(QFileInfo::exists(batchFilePath))) {
        const bool runSucceeded = runClangBatchFile(batchFilePath);
        QCoreApplication::exit(!runSucceeded);
    }
#endif
}

#ifdef WITH_TESTS
QList<QObject *> ClangCodeModelPlugin::createTestObjects() const
{
    return {
        new Tests::ClangCodeCompletionTest,
    };
}
#endif


} // namespace Internal
} // namespace Clang
