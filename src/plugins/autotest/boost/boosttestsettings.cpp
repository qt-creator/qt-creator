/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "boosttestsettings.h"

namespace Autotest {
namespace Internal {

static const char logLevelKey[]         = "LogLevel";
static const char reportLevelKey[]      = "ReportLevel";
static const char seedKey[]             = "Seed";
static const char randomizeKey[]        = "Randomize";
static const char systemErrorsKey[]     = "SystemErrors";
static const char fpExceptionsKey[]     = "FPExceptions";
static const char memLeaksKey[]         = "MemoryLeaks";

QString BoostTestSettings::name() const
{
    return QString("BoostTest");
}

void BoostTestSettings::fromFrameworkSettings(const QSettings *s)

{
    logLevel = LogLevel((s->value(logLevelKey, int(LogLevel::All)).toInt()));
    reportLevel = ReportLevel((s->value(reportLevelKey, int(ReportLevel::Confirm)).toInt()));
    systemErrors = s->value(systemErrorsKey, false).toBool();
    fpExceptions = s->value(fpExceptionsKey, false).toBool();
    memLeaks = s->value(memLeaksKey, true).toBool();
    randomize = s->value(randomizeKey, false).toBool();
    seed = s->value(seedKey, 0).toInt();
}

void BoostTestSettings::toFrameworkSettings(QSettings *s) const
{
    s->setValue(logLevelKey, int(logLevel));
    s->setValue(reportLevelKey, int(reportLevel));
    s->setValue(systemErrorsKey, systemErrors);
    s->setValue(fpExceptionsKey, fpExceptions);
    s->setValue(memLeaksKey, memLeaks);
    s->setValue(randomizeKey, randomize);
    s->setValue(seedKey, seed);
}

QString BoostTestSettings::logLevelToOption(const LogLevel logLevel)
{
    switch (logLevel) {
    case LogLevel::All: return QString("all");
    case LogLevel::Success: return QString("success");
    case LogLevel::TestSuite: return QString("test_suite");
    case LogLevel::UnitScope: return QString("unit_scope");
    case LogLevel::Message: return QString("message");
    case LogLevel::Error: return QString("error");
    case LogLevel::CppException: return QString("cpp_exception");
    case LogLevel::SystemError: return QString("system_error");
    case LogLevel::FatalError: return QString("fatal_error");
    case LogLevel::Nothing: return QString("nothing");
    case LogLevel::Warning: return QString("warning");
    }
    return QString();
}

QString BoostTestSettings::reportLevelToOption(const ReportLevel reportLevel)
{
    switch (reportLevel) {
    case ReportLevel::Confirm: return QString("confirm");
    case ReportLevel::Short: return QString("short");
    case ReportLevel::Detailed: return QString("detailed");
    case ReportLevel::No: return QString("no");
    }
    return QString();
}

} // namespace Internal
} // namespace Autotest
