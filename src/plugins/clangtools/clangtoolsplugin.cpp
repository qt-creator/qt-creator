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

#include "clangstaticanalyzerconfigwidget.h"
#include "clangtoolsconstants.h"
#include "clangstaticanalyzerprojectsettingswidget.h"
#include "clangstaticanalyzerruncontrol.h"
#include "clangstaticanalyzertool.h"
#include "clangtidyclazytool.h"

#ifdef WITH_TESTS
#include "clangstaticanalyzerpreconfiguredsessiontests.h"
#include "clangstaticanalyzerunittests.h"
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

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

#include <QtPlugin>

using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

class ClangStaticAnalyzerOptionsPage : public Core::IOptionsPage
{
public:
    explicit ClangStaticAnalyzerOptionsPage()
    {
        setId("Analyzer.ClangStaticAnalyzer.Settings"); // TODO: Get it from "clangstaticanalyzersettings.h"
        setDisplayName(QCoreApplication::translate(
                           "ClangTools::Internal::ClangStaticAnalyzerOptionsPage",
                           "Clang Static Analyzer"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    }

    QWidget *widget()
    {
        if (!m_widget)
            m_widget = new ClangStaticAnalyzerConfigWidget(ClangToolsSettings::instance());
        return m_widget;
    }

    void apply()
    {
        ClangToolsSettings::instance()->writeSettings();
    }

    void finish()
    {
        delete m_widget;
    }

private:
    QPointer<QWidget> m_widget;
};

class ClangToolsPluginPrivate
{
public:
    ClangStaticAnalyzerTool staticAnalyzerTool;
    ClangTidyClazyTool clangTidyClazyTool;
    ClangStaticAnalyzerOptionsPage optionsPage;
};

ClangToolsPlugin::~ClangToolsPlugin()
{
    delete d;
}

bool ClangToolsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    d = new ClangToolsPluginPrivate;

    auto panelFactory = new ProjectPanelFactory();
    panelFactory->setPriority(100);
    panelFactory->setDisplayName(tr("Clang Tools"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new ProjectSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    return true;
}

QList<QObject *> ClangToolsPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new ClangStaticAnalyzerPreconfiguredSessionTests;
    tests << new ClangStaticAnalyzerUnitTests;
#endif
    return tests;
}

} // namespace Internal
} // namespace ClangTools
