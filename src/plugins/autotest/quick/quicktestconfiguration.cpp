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

#include "quicktestconfiguration.h"
#include "../qtest/qttestconstants.h"
#include "../qtest/qttestoutputreader.h"
#include "../qtest/qttestsettings.h"
#include "../qtest/qttest_utils.h"
#include "../autotestplugin.h"
#include "../testframeworkmanager.h"
#include "../testsettings.h"

namespace Autotest {
namespace Internal {

QuickTestConfiguration::QuickTestConfiguration()
{
    setMixedDebugging(true);
}

TestOutputReader *QuickTestConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                       QProcess *app) const
{
    static const Core::Id id
            = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QtTest::Constants::FRAMEWORK_NAME);
    TestFrameworkManager *manager = TestFrameworkManager::instance();
    auto qtSettings = qSharedPointerCast<QtTestSettings>(manager->settingsForTestFramework(id));
    if (qtSettings.isNull())
        return nullptr;
    if (qtSettings->useXMLOutput)
        return new QtTestOutputReader(fi, app, buildDirectory(), projectFile(), QtTestOutputReader::XML);
    else
        return new QtTestOutputReader(fi, app, buildDirectory(), projectFile(), QtTestOutputReader::PlainText);
}

QStringList QuickTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    static const Core::Id id
            = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QtTest::Constants::FRAMEWORK_NAME);

    QStringList arguments;
    if (AutotestPlugin::instance()->settings()->processArgs) {
        arguments.append(QTestUtils::filterInterfering
                         (runnable().commandLineArguments.split(' ', QString::SkipEmptyParts),
                          omitted, true));
    }

    TestFrameworkManager *manager = TestFrameworkManager::instance();
    auto qtSettings = qSharedPointerCast<QtTestSettings>(manager->settingsForTestFramework(id));
    if (qtSettings.isNull())
        return arguments;
    if (qtSettings->useXMLOutput)
        arguments << "-xml";
    if (testCases().count())
        arguments << testCases();

    const QString &metricsOption = QtTestSettings::metricsTypeToOption(qtSettings->metrics);
    if (!metricsOption.isEmpty())
        arguments << metricsOption;

    if (isDebugRunMode()) {
        if (qtSettings->noCrashHandler)
            arguments << "-nocrashhandler";
    }
    return arguments;
}

void QuickTestConfiguration::setUnnamedOnly(bool unnamedOnly)
{
    m_unnamedOnly = unnamedOnly;
}

} // namespace Internal
} // namespace Autotest
