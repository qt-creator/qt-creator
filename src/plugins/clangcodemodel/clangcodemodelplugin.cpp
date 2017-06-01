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

#ifdef WITH_TESTS
#  include "test/clangbatchfileprocessor.h"
#  include "test/clangcodecompletion_test.h"
#endif

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <texteditor/textmark.h>

namespace ClangCodeModel {
namespace Internal {

namespace {

void addProjectPanelWidget()
{
    auto panelFactory = new ProjectExplorer::ProjectPanelFactory();
    panelFactory->setPriority(60);
    panelFactory->setDisplayName(ClangProjectSettingsWidget::tr("Clang Code Model"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ClangProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
}

} // anonymous namespace

bool ClangCodeModelPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            &ProjectExplorer::ProjectExplorerPlugin::finishedInitialization,
            this,
            &ClangCodeModelPlugin::maybeHandleBatchFileAndExit);

    CppTools::CppModelManager::instance()->activateClangCodeModel(&m_modelManagerSupportProvider);

    addProjectPanelWidget();

    return true;
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
