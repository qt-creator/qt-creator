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
    return QLatin1String("clang" QTC_HOST_EXE_SUFFIX);
}

QString ClangStaticAnalyzerSettings::defaultClangExecutable() const
{
    const QString shippedBinary = Core::ICore::libexecPath()
                    + QLatin1String("/clang/bin/")
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
