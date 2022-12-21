// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "valgrindconfigwidget.h"
#include "valgrindsettings.h"
#include "valgrindtr.h"

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

class ValgrindRunConfigurationAspect : public GlobalOrProjectAspect
{
public:
    ValgrindRunConfigurationAspect(Target *)
    {
        setProjectSettings(new ValgrindProjectSettings);
        setGlobalSettings(ValgrindGlobalSettings::instance());
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(Tr::tr("Valgrind Settings"));
        setUsingGlobalSettings(true);
        resetProjectToGlobalSettings();
        setConfigWidgetCreator([this] { return new Debugger::AnalyzerRunConfigWidget(this); });
    }
};

class ValgrindPluginPrivate
{
public:
    ValgrindGlobalSettings valgrindGlobalSettings; // Needs to come before the tools.
    MemcheckTool memcheckTool;
    CallgrindTool callgrindTool;
    ValgrindOptionsPage valgrindOptionsPage;
};

ValgrindPlugin::~ValgrindPlugin()
{
    delete d;
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    d = new ValgrindPluginPrivate;

    RunConfiguration::registerAspect<ValgrindRunConfigurationAspect>();

    return true;
}

QVector<QObject *> ValgrindPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new Test::ValgrindMemcheckParserTest << new Test::ValgrindTestRunnerTest;
#endif
    return tests;
}

} // namespace Internal
} // namespace Valgrind
