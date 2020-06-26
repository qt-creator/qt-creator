/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include "cppcheckplugin.h"
#include "cppcheckconstants.h"
#include "cppcheckdiagnosticview.h"
#include "cppchecktextmarkmanager.h"
#include "cppchecktool.h"
#include "cppchecktrigger.h"
#include "cppcheckdiagnosticsmodel.h"
#include "cppcheckmanualrundialog.h"

#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/debuggermainwindow.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

namespace Cppcheck {
namespace Internal {

class CppcheckPluginPrivate final : public QObject
{
public:
    explicit CppcheckPluginPrivate();
    CppcheckTextMarkManager marks;
    CppcheckTool tool;
    CppcheckTrigger trigger;
    CppcheckOptionsPage options;
    DiagnosticsModel manualRunModel;
    CppcheckTool manualRunTool;
    Utils::Perspective perspective{Constants::PERSPECTIVE_ID, CppcheckPlugin::tr("Cppcheck")};
    QAction *manualRunAction;

    void startManualRun();
    void updateManualRunAction();
};

CppcheckPluginPrivate::CppcheckPluginPrivate() :
    tool(marks, Constants::CHECK_PROGRESS_ID),
    trigger(marks, tool),
    options(tool, trigger),
    manualRunTool(manualRunModel, Constants::MANUAL_CHECK_PROGRESS_ID)
{
    manualRunTool.updateOptions(tool.options());

    auto manualRunView = new DiagnosticView;
    manualRunView->setModel(&manualRunModel);
    perspective.addWindow(manualRunView, Utils::Perspective::SplitVertical, nullptr);

    {
        // Go to previous diagnostic
        auto action = new QAction(this);
        action->setEnabled(false);
        action->setIcon(Utils::Icons::PREV_TOOLBAR.icon());
        action->setToolTip(CppcheckPlugin::tr("Go to previous diagnostic."));
        connect(action, &QAction::triggered,
                manualRunView, &Debugger::DetailedErrorView::goBack);
        connect (&manualRunModel, &DiagnosticsModel::hasDataChanged,
                action, &QAction::setEnabled);
        perspective.addToolBarAction(action);
    }

    {
        // Go to next diagnostic
        auto action = new QAction(this);
        action->setEnabled(false);
        action->setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
        action->setToolTip(CppcheckPlugin::tr("Go to next diagnostic."));
        connect(action, &QAction::triggered,
                manualRunView, &Debugger::DetailedErrorView::goNext);
        connect (&manualRunModel, &DiagnosticsModel::hasDataChanged,
                action, &QAction::setEnabled);
        perspective.addToolBarAction(action);
    }

    {
        // Clear data
        auto action = new QAction(this);
        action->setEnabled(false);
        action->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
        action->setToolTip(CppcheckPlugin::tr("Clear"));
        connect(action, &QAction::triggered,
                &manualRunModel, &DiagnosticsModel::clear);
        connect (&manualRunModel, &DiagnosticsModel::hasDataChanged,
                action, &QAction::setEnabled);
        perspective.addToolBarAction(action);
    }
}

void CppcheckPluginPrivate::startManualRun() {
    auto project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return;

    ManualRunDialog dialog(manualRunTool.options(), project);
    if (dialog.exec() == ManualRunDialog::Rejected)
        return;

    manualRunModel.clear();

    const auto files = dialog.filePaths();
    if (files.isEmpty())
        return;

    manualRunTool.setProject(project);
    manualRunTool.updateOptions(dialog.options());
    manualRunTool.check(files);
    perspective.select();
}

void CppcheckPluginPrivate::updateManualRunAction()
{
    using namespace ProjectExplorer;
    const Project *project = SessionManager::startupProject();
    const Target *target = SessionManager::startupTarget();
    const Utils::Id cxx = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
    const bool canRun = target && project->projectLanguages().contains(cxx)
                  && ToolChainKitAspect::cxxToolChain(target->kit());
    manualRunAction->setEnabled(canRun);
}

CppcheckPlugin::CppcheckPlugin() = default;

CppcheckPlugin::~CppcheckPlugin() = default;

bool CppcheckPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d.reset(new CppcheckPluginPrivate);

    using namespace Core;
    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);

    {
        auto action = new QAction(tr("Cppcheck..."), this);
        menu->addAction(ActionManager::registerAction(action, Constants::MANUAL_RUN_ACTION),
                        Debugger::Constants::G_ANALYZER_TOOLS);
        connect(action, &QAction::triggered,
                d.get(), &CppcheckPluginPrivate::startManualRun);
        d->manualRunAction = action;
    }

    using ProjectExplorer::ProjectExplorerPlugin;
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            d.get(), &CppcheckPluginPrivate::updateManualRunAction);
    d->updateManualRunAction();

    return true;
}

} // namespace Internal
} // namespace Cppcheck
