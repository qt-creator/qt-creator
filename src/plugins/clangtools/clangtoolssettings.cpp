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

#include "clangtoolssettings.h"

#include "clangtoolsconstants.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QThread>

static const char simultaneousProcessesKey[] = "simultaneousProcesses";
static const char buildBeforeAnalysisKey[] = "buildBeforeAnalysis";
static const char diagnosticConfigIdKey[] = "diagnosticConfigId";

namespace ClangTools {
namespace Internal {

ClangToolsSettings::ClangToolsSettings()
{
    readSettings();
}

ClangToolsSettings *ClangToolsSettings::instance()
{
    static ClangToolsSettings instance;
    return &instance;
}

int ClangToolsSettings::savedSimultaneousProcesses() const
{
    return m_savedSimultaneousProcesses;
}

int ClangToolsSettings::simultaneousProcesses() const
{
    return m_simultaneousProcesses;
}

void ClangToolsSettings::setSimultaneousProcesses(int processes)
{
    m_simultaneousProcesses = processes;
}

bool ClangToolsSettings::savedBuildBeforeAnalysis() const
{
    return m_savedBuildBeforeAnalysis;
}

bool ClangToolsSettings::buildBeforeAnalysis() const
{
    return m_buildBeforeAnalysis;
}

void ClangToolsSettings::setBuildBeforeAnalysis(bool build)
{
    m_buildBeforeAnalysis = build;
}

Core::Id ClangToolsSettings::savedDiagnosticConfigId() const
{
    return m_savedDiagnosticConfigId;
}

Core::Id ClangToolsSettings::diagnosticConfigId() const
{
    return m_diagnosticConfigId;
}

void ClangToolsSettings::setDiagnosticConfigId(Core::Id id)
{
    m_diagnosticConfigId = id;
}

void ClangToolsSettings::updateSavedBuildBeforeAnalysiIfRequired()
{
    if (m_savedBuildBeforeAnalysis == m_buildBeforeAnalysis)
        return;
    m_savedBuildBeforeAnalysis = m_buildBeforeAnalysis;
    emit buildBeforeAnalysisChanged(m_savedBuildBeforeAnalysis);
}

void ClangToolsSettings::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));

    const int defaultSimultaneousProcesses = qMax(0, QThread::idealThreadCount() / 2);
    m_savedSimultaneousProcesses = m_simultaneousProcesses
            = settings->value(QString(simultaneousProcessesKey),
                              defaultSimultaneousProcesses).toInt();

    m_buildBeforeAnalysis = settings->value(QString(buildBeforeAnalysisKey), true).toBool();

    m_diagnosticConfigId = Core::Id::fromSetting(settings->value(QString(diagnosticConfigIdKey)));
    if (!m_diagnosticConfigId.isValid())
        m_diagnosticConfigId = "Builtin.TidyAndClazy";

    m_savedDiagnosticConfigId = m_diagnosticConfigId;

    updateSavedBuildBeforeAnalysiIfRequired();

    settings->endGroup();
}

void ClangToolsSettings::writeSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QString(Constants::SETTINGS_ID));
    settings->setValue(QString(simultaneousProcessesKey), m_simultaneousProcesses);
    settings->setValue(QString(buildBeforeAnalysisKey), m_buildBeforeAnalysis);
    settings->setValue(QString(diagnosticConfigIdKey), m_diagnosticConfigId.toSetting());

    m_savedSimultaneousProcesses = m_simultaneousProcesses;
    m_savedDiagnosticConfigId = m_diagnosticConfigId;
    updateSavedBuildBeforeAnalysiIfRequired();

    settings->endGroup();
}

} // namespace Internal
} // namespace ClangTools
