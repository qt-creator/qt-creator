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

#include "gtestconfiguration.h"
#include "gtestoutputreader.h"
#include "../testsettings.h"

namespace Autotest {
namespace Internal {

TestOutputReader *GTestConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                   QProcess *app) const
{
    return new GTestOutputReader(fi, app, buildDirectory());
}

QStringList GTestConfiguration::argumentsForTestRunner(const TestSettings &settings) const
{
    QStringList arguments;
    const QStringList &testSets = testCases();
    if (testSets.size())
        arguments << QLatin1String("--gtest_filter=") + testSets.join(QLatin1Char(':'));
    if (settings.gtestRunDisabled)
        arguments << QLatin1String("--gtest_also_run_disabled_tests");
    if (settings.gtestRepeat)
        arguments << QString::fromLatin1("--gtest_repeat=%1").arg(settings.gtestIterations);
    if (settings.gtestShuffle) {
        arguments << QLatin1String("--gtest_shuffle")
                  << QString::fromLatin1("--gtest_random_seed=%1").arg(settings.gtestSeed);
    }
    return arguments;
}

} // namespace Internal
} // namespace Autotest
