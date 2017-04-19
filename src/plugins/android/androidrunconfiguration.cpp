/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunconfiguration.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androidmanager.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QToolButton>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

using namespace ProjectExplorer;

namespace Android {

static QRegularExpression logCatRegExp("([0-9\\-]*\\s+[0-9\\-:.]*)"     // 1. time
                                        "\\s*"
                                        "([DEIVWF])"                    // 2. log level
                                        "\\/"
                                        "(.*)"                          // 3. TAG
                                        "\\(\\s*"
                                        "(\\d+)"                        // 4. PID
                                        "\\)\\:\\s"
                                        "(.*)");                        // 5. Message

AndroidOutputFormatter::AndroidOutputFormatter(Project *project)
 : QtSupport::QtOutputFormatter(project)
 , m_filtersButton(new QToolButton)
{
    auto filtersMenu = new QMenu(m_filtersButton.data());

    m_filtersButton->setToolTip(tr("Filters"));
    m_filtersButton->setIcon(Utils::Icons::FILTER.icon());
    m_filtersButton->setProperty("noArrow", true);
    m_filtersButton->setAutoRaise(true);
    m_filtersButton->setPopupMode(QToolButton::InstantPopup);
    m_filtersButton->setMenu(filtersMenu);

    auto logsMenu = filtersMenu->addMenu(tr("Log Level"));
    addLogAction(All, logsMenu, tr("All"));
    addLogAction(Verbose, logsMenu, tr("Verbose"));
    addLogAction(Info, logsMenu, tr("Info"));
    addLogAction(Debug, logsMenu, tr("Debug"));
    addLogAction(Warning, logsMenu, tr("Warning"));
    addLogAction(Error, logsMenu, tr("Error"));
    addLogAction(Fatal, logsMenu, tr("Fatal"));
    updateLogMenu();
    m_appsMenu = filtersMenu->addMenu(tr("Applications"));
    appendPid(-1, tr("All"));
}

AndroidOutputFormatter::~AndroidOutputFormatter()
{}

QList<QWidget *> AndroidOutputFormatter::toolbarWidgets() const
{
    return QList<QWidget *>{m_filtersButton.data()};
}

void AndroidOutputFormatter::appendMessage(const QString &text, Utils::OutputFormat format)
{
    if (text.isEmpty())
        return;

    CachedLine line;
    line.content = text;

    if (format < Utils::StdOutFormat) {
        line.level = SkipFiltering;
        line.pid = -1;
    } else {
        QRegularExpressionMatch match = logCatRegExp.match(text);
        if (!match.hasMatch())
            return;
        line.level = None;

        switch (match.captured(2).toLatin1()[0]) {
        case 'D': line.level = Debug; break;
        case 'I': line.level = Info; break;
        case 'V': line.level = Verbose; break;
        case 'W': line.level = Warning; break;
        case 'E': line.level = Error; break;
        case 'F': line.level = Fatal; break;
        default: return;
        }
        line.pid = match.captured(4).toLongLong();
    }

    m_cachedLines.append(line);
    if (m_cachedLines.size() > ProjectExplorerPlugin::projectExplorerSettings().maxAppOutputLines)
        m_cachedLines.pop_front();

    filterMessage(line);
}

void AndroidOutputFormatter::clear()
{
    m_cachedLines.clear();
}

void AndroidOutputFormatter::appendPid(qint64 pid, const QString &name)
{
    if (m_pids.contains(pid))
        return;

    auto action = m_appsMenu->addAction(name);
    m_pids[pid] = action;
    action->setCheckable(true);
    action->setChecked(pid != -1);
    connect(action, &QAction::triggered, this, &AndroidOutputFormatter::applyFilter);
    applyFilter();
}

void AndroidOutputFormatter::removePid(qint64 pid)
{
    if (pid == -1) {
        for (auto action : m_pids)
            m_appsMenu->removeAction(action);
        m_pids.clear();
    } else {
        m_appsMenu->removeAction(m_pids[pid]);
        m_pids.remove(pid);
    }
}

void AndroidOutputFormatter::updateLogMenu(LogLevel set, LogLevel reset)
{
    m_logLevelFlags |= set;
    m_logLevelFlags &= ~reset;
    for (const auto & pair : m_logLevels)
        pair.second->setChecked((m_logLevelFlags & pair.first) == pair.first);

    applyFilter();
}

void AndroidOutputFormatter::filterMessage(const CachedLine &line)
{
    if (line.level == SkipFiltering || m_pids[-1]->isChecked()) {
        QtOutputFormatter::appendMessage(line.content, Utils::NormalMessageFormat);
    } else {
        // Filter Log Level
        if (!(m_logLevelFlags & line.level))
            return;

        // Filter PIDs
        if (!m_pids[-1]->isChecked()) {
            auto it = m_pids.find(line.pid);
            if (it == m_pids.end() || !(*it)->isChecked())
                return;
        }

        Utils::OutputFormat format = Utils::NormalMessageFormat;
        switch (line.level) {
        case Debug:
            format = Utils::DebugFormat;
            break;
        case Info:
        case Verbose:
            format = Utils::StdOutFormat;
            break;

        case Warning:
        case Error:
        case Fatal:
            format = Utils::StdErrFormat;
            break;
        default:
            return;
        }

        Utils::OutputFormatter::appendMessage(line.content, format);
    }
}

void AndroidOutputFormatter::applyFilter()
{
    if (!plainTextEdit())
        return;

    plainTextEdit()->clear();
    if (!m_pids[-1]->isChecked()) {
        bool allOn = true;
        for (auto action : m_pids) {
            if (!action->isChecked()) {
                allOn = false;
                break;
            }
        }
        m_pids[-1]->setChecked(allOn);
    } else {
        for (auto action : m_pids)
            action->setChecked(true);
    }

    for (const auto &line : m_cachedLines)
        filterMessage(line);
}

AndroidRunConfiguration::AndroidRunConfiguration(Target *parent, Core::Id id)
    : RunConfiguration(parent, id)
{
}

AndroidRunConfiguration::AndroidRunConfiguration(Target *parent, AndroidRunConfiguration *source)
    : RunConfiguration(parent, source)
{
}

QWidget *AndroidRunConfiguration::createConfigurationWidget()
{
    return 0;// no special running configurations
}

Utils::OutputFormatter *AndroidRunConfiguration::createOutputFormatter() const
{
    return new AndroidOutputFormatter(target()->project());
}

const QString AndroidRunConfiguration::remoteChannel() const
{
    return QLatin1String(":5039");
}

} // namespace Android
