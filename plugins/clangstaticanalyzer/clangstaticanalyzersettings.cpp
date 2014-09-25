/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzersettings.h"

#include "clangstaticanalyzerconstants.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QThread>

static const char clangExecutableKey[] = "clangExecutable";
static const char simultaneousProcessesKey[] = "simultaneousProcesses";

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerSettings::ClangStaticAnalyzerSettings()
    : m_simultaneousProcesses(-1)
{
    readSettings();
}

ClangStaticAnalyzerSettings *ClangStaticAnalyzerSettings::instance()
{
    static ClangStaticAnalyzerSettings instance;
    return &instance;
}

QString ClangStaticAnalyzerSettings::clangExecutable() const
{
    return m_clangExecutable;
}

void ClangStaticAnalyzerSettings::setClangExecutable(const QString &exectuable)
{
    QTC_ASSERT(!exectuable.isEmpty(), return);
    m_clangExecutable = exectuable;
}

int ClangStaticAnalyzerSettings::simultaneousProcesses() const
{
    return m_simultaneousProcesses;
}

void ClangStaticAnalyzerSettings::setSimultaneousProcesses(int processes)
{
    QTC_ASSERT(processes >=1, return);
    m_simultaneousProcesses = processes;
}

void ClangStaticAnalyzerSettings::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));

    const QString defaultClangExecutable = Utils::HostOsInfo::withExecutableSuffix(
                QLatin1String(Constants::CLANG_EXECUTABLE_BASE_NAME));
    setClangExecutable(settings->value(QLatin1String(clangExecutableKey),
                                       defaultClangExecutable).toString());

    const int defaultSimultaneousProcesses = qMax(0, QThread::idealThreadCount() / 2);
    setSimultaneousProcesses(settings->value(QLatin1String(simultaneousProcessesKey),
                                             defaultSimultaneousProcesses).toInt());

    settings->endGroup();
}

void ClangStaticAnalyzerSettings::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));
    settings->setValue(QLatin1String(clangExecutableKey), clangExecutable());
    settings->setValue(QLatin1String(simultaneousProcessesKey), simultaneousProcesses());
    settings->endGroup();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
