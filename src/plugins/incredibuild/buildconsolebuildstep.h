/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include "commandbuilder.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildsteplist.h>

#include <QList>

namespace IncrediBuild {
namespace Internal {

class BuildConsoleBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    explicit BuildConsoleBuildStep(ProjectExplorer::BuildStepList *buildStepList, Utils::Id id);
    ~BuildConsoleBuildStep() override;

    bool init() override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool avoidLocal() const { return m_avoidLocal; }
    void avoidLocal(bool avoidLocal) { m_avoidLocal = avoidLocal; }

    const QString &profileXml() const { return m_profileXml; }
    void profileXml(const QString &profileXml) { m_profileXml = profileXml; }

    int maxCpu() const { return m_maxCpu; }
    void maxCpu(int maxCpu) { m_maxCpu = maxCpu; }

    const QStringList& supportedWindowsVersions() const;
    const QString &maxWinVer() const { return m_maxWinVer; }
    void maxWinVer(const QString &maxWinVer) { m_maxWinVer = maxWinVer; }

    const QString &minWinVer() const { return m_minWinVer; }
    void minWinVer(const QString &minWinVer) { m_minWinVer = minWinVer; }

    const QString &title() const { return m_title; }
    void title(const QString &title) { m_title = title; }

    const QString &monFile() const { return m_monFile; }
    void monFile(const QString &monFile) { m_monFile = monFile; }

    bool suppressStdOut() const { return m_suppressStdOut; }
    void suppressStdOut(bool suppressStdOut) { m_suppressStdOut = suppressStdOut; }

    const QString &logFile() const { return m_logFile; }
    void logFile(const QString &logFile) { m_logFile = logFile; }

    bool showCmd() const { return m_showCmd; }
    void showCmd(bool showCmd) { m_showCmd = showCmd; }

    bool showAgents() const { return m_showAgents; }
    void showAgents(bool showAgents) { m_showAgents = showAgents; }

    bool showTime() const { return m_showTime; }
    void showTime(bool showTime) { m_showTime = showTime; }

    bool hideHeader() const { return m_hideHeader; }
    void hideHeader(bool hideHeader) { m_hideHeader = hideHeader; }

    const QStringList& supportedLogLevels() const;
    const QString &logLevel() const { return m_logLevel; }
    void logLevel(const QString &logLevel) { m_logLevel = logLevel; }

    const QString &setEnv() const { return m_setEnv; }
    void setEnv(const QString &setEnv) { m_setEnv = setEnv; }

    bool stopOnError() const { return m_stopOnError; }
    void stopOnError(bool stopOnError) { m_stopOnError = stopOnError; }

    const QString &additionalArguments() const { return m_additionalArguments; }
    void additionalArguments(const QString &additionalArguments) { m_additionalArguments = additionalArguments; }

    bool openMonitor() const { return m_openMonitor; }
    void openMonitor(bool openMonitor) { m_openMonitor = openMonitor; }

    bool keepJobNum() const { return m_keepJobNum; }
    void keepJobNum(bool keepJobNum) { m_keepJobNum = keepJobNum; }

    const QStringList& supportedCommandBuilders();
    CommandBuilder *commandBuilder() const { return m_activeCommandBuilder; }
    void commandBuilder(const QString &commandBuilder);

    bool loadedFromMap() const { return m_loadedFromMap; }
    void tryToMigrate();

    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;

private:
    void initCommandBuilders();
    QString normalizeWinVerArgument(QString winVer);

    ProjectExplorer::BuildStepList *m_earlierSteps{};
    bool m_loadedFromMap{false};
    bool m_avoidLocal{false};
    QString m_profileXml{};
    int m_maxCpu{0};
    QString m_maxWinVer{};
    QString m_minWinVer{};
    QString m_title{};
    QString m_monFile{};
    bool m_suppressStdOut{false};
    QString m_logFile{};
    bool m_showCmd{false};
    bool m_showAgents{false};
    bool m_showTime{false};
    bool m_hideHeader{false};
    QString m_logLevel{};
    QString m_setEnv{};
    bool m_stopOnError{false};
    QString m_additionalArguments{};
    bool m_openMonitor{true};
    bool m_keepJobNum{false};
    CommandBuilder* m_activeCommandBuilder{};
    QList<CommandBuilder*> m_commandBuildersList{};
};

} // namespace Internal
} // namespace IncrediBuild
