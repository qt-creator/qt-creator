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

#include "clangtoolsplugin.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolsprojectsettingswidget.h"
#include "settingswidget.h"

#ifdef WITH_TESTS
#include "clangtoolspreconfiguredsessiontests.h"
#include "clangtoolsunittests.h"
#endif

#include <debugger/analyzer/analyzericons.h>

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

static ProjectPanelFactory *m_projectPanelFactoryInstance = nullptr;

ProjectPanelFactory *projectPanelFactory()
{
    return m_projectPanelFactoryInstance;
}

class ClangToolsOptionsPage : public IOptionsPage
{
public:
    ClangToolsOptionsPage()
    {
        setId(Constants::SETTINGS_PAGE_ID);
        setDisplayName(QCoreApplication::translate(
                           "ClangTools::Internal::ClangToolsOptionsPage",
                           "Clang Tools"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    }

    QWidget *widget() override
    {
        if (!m_widget)
            m_widget = new SettingsWidget(ClangToolsSettings::instance());
        return m_widget;
    }

    void apply() override { m_widget->apply(); }
    void finish() override { delete m_widget; }

private:
    QPointer<SettingsWidget> m_widget;
};

class ClangToolsPluginPrivate
{
public:
    ClangTool clangTool;
    ClangToolsOptionsPage optionsPage;
    ClangToolsProjectSettingsManager settingsManager;
};

ClangToolsPlugin::~ClangToolsPlugin()
{
    delete d;
}

bool ClangToolsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Import tidy/clazy diagnostic configs from CppTools now
    // instead of at opening time of the settings page
    ClangToolsSettings::instance();

    d = new ClangToolsPluginPrivate;

    ActionManager::registerAction(d->clangTool.startAction(), Constants::RUN_ON_PROJECT);
    ActionManager::registerAction(d->clangTool.startOnCurrentFileAction(),
                                  Constants::RUN_ON_CURRENT_FILE);

    auto panelFactory = m_projectPanelFactoryInstance = new ProjectPanelFactory;
    panelFactory->setPriority(100);
    panelFactory->setId(Constants::PROJECT_PANEL_ID);
    panelFactory->setDisplayName(tr("Clang Tools"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new ProjectSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    return true;
}

QVector<QObject *> ClangToolsPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new PreconfiguredSessionTests;
    tests << new ClangToolsUnitTests;
#endif
    return tests;
}

} // namespace Internal
} // namespace ClangTools
