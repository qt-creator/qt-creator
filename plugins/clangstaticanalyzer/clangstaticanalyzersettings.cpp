/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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

static QString clangExecutableFileName()
{
    return QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "clang-cl.exe" : "clang");
}

QString ClangStaticAnalyzerSettings::defaultClangExecutable() const
{
    const QString shippedBinary = Core::ICore::libexecPath() + QLatin1Char('/')
            + clangExecutableFileName();
    if (QFileInfo(shippedBinary).isExecutable())
        return shippedBinary;
    return clangExecutableFileName();
}

QString ClangStaticAnalyzerSettings::clangExecutable(bool *isSet) const
{
    if (m_clangExecutable.isEmpty()) {
        if (isSet)
            *isSet = false;
        return defaultClangExecutable();
    }
    if (isSet)
        *isSet = true;
    return m_clangExecutable;
}

void ClangStaticAnalyzerSettings::setClangExecutable(const QString &exectuable)
{
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

    setClangExecutable(settings->value(QLatin1String(clangExecutableKey)).toString());

    const int defaultSimultaneousProcesses = qMax(0, QThread::idealThreadCount() / 2);
    setSimultaneousProcesses(settings->value(QLatin1String(simultaneousProcessesKey),
                                             defaultSimultaneousProcesses).toInt());

    settings->endGroup();
}

void ClangStaticAnalyzerSettings::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));
    settings->setValue(QLatin1String(clangExecutableKey), m_clangExecutable);
    settings->setValue(QLatin1String(simultaneousProcessesKey), simultaneousProcesses());
    settings->endGroup();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
