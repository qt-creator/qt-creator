/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "valgrindsettings.h"
#include "valgrindconfigwidget.h"

#ifdef WITH_TESTS
#   include "valgrindmemcheckparsertest.h"
#   include "valgrindtestrunnertest.h"
#endif

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <debugger/analyzer/analyzerrunconfigwidget.h>
#include <debugger/analyzer/analyzericons.h>

#include <projectexplorer/projectexplorer.h>

#include <QCoreApplication>
#include <QPointer>

using namespace Core;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

static ValgrindGlobalSettings *theGlobalSettings = nullptr;

class ValgrindOptionsPage : public IOptionsPage
{
public:
    explicit ValgrindOptionsPage(QObject *parent)
        : IOptionsPage(parent)
    {
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindOptionsPage", "Valgrind"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    }

    QWidget *widget() override
    {
        if (!m_widget)
            m_widget = new ValgrindConfigWidget(theGlobalSettings, true);
        return m_widget;
    }

    void apply() override
    {
        theGlobalSettings->writeSettings();
    }

    void finish() override
    {
        delete m_widget;
    }

private:
    QPointer<QWidget> m_widget;
};

class ValgrindRunConfigurationAspect : public GlobalOrProjectAspect
{
public:
    ValgrindRunConfigurationAspect(Target *)
    {
        setProjectSettings(new ValgrindProjectSettings);
        setGlobalSettings(ValgrindPlugin::globalSettings());
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindRunConfigurationAspect",
                                                   "Valgrind Settings"));
        setUsingGlobalSettings(true);
        resetProjectToGlobalSettings();
        setConfigWidgetCreator([this] { return new Debugger::AnalyzerRunConfigWidget(this); });
    }
};

ValgrindPlugin::~ValgrindPlugin()
{
    delete theGlobalSettings;
    theGlobalSettings = nullptr;
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    theGlobalSettings = new ValgrindGlobalSettings;
    theGlobalSettings->readSettings();

    new ValgrindOptionsPage(this);

    RunConfiguration::registerAspect<ValgrindRunConfigurationAspect>();

    return true;
}

void ValgrindPlugin::extensionsInitialized()
{
    initMemcheckTool();
    initCallgrindTool();
}

ExtensionSystem::IPlugin::ShutdownFlag ValgrindPlugin::aboutToShutdown()
{
    destroyCallgrindTool();
    destroyMemcheckTool();
    return SynchronousShutdown;
}

ValgrindGlobalSettings *ValgrindPlugin::globalSettings()
{
    return theGlobalSettings;
}

QList<QObject *> ValgrindPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new Test::ValgrindMemcheckParserTest << new Test::ValgrindTestRunnerTest;
#endif
    return tests;
}

} // namespace Internal
} // namespace Valgrind
