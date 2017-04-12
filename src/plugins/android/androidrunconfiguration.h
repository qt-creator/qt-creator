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

#pragma once

#include "android_global.h"

#include <projectexplorer/runconfiguration.h>
#include <qtsupport/qtoutputformatter.h>

#include <QMenu>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Android {

class AndroidOutputFormatter : public QtSupport::QtOutputFormatter
{
    Q_OBJECT
public:
    enum LogLevel {
        None = 0,
        Verbose = 1,
        Info = 1 << 1,
        Debug = 1 << 2,
        Warning = 1 << 3,
        Error = 1 << 4,
        Fatal = 1 << 5,
        All = Verbose | Info | Debug | Warning | Error | Fatal,
        SkipFiltering = ~All
    };

public:
    explicit AndroidOutputFormatter(ProjectExplorer::Project *project);
    ~AndroidOutputFormatter();

    // OutputFormatter interface
    QList<QWidget*> toolbarWidgets() const override;
    void appendMessage(const QString &text, Utils::OutputFormat format) override;
    void clear() override;

public slots:
    void appendPid(qint64 pid, const QString &name);
    void removePid(qint64 pid);

private:
    struct CachedLine {
        qint64 pid;
        LogLevel level;
        QString content;
    };

private:
    void updateLogMenu(LogLevel set = None, LogLevel reset = None);
    void filterMessage(const CachedLine &line);

    void applyFilter();
    void addLogAction(LogLevel level, QMenu *logsMenu, const QString &name) {
        auto action = logsMenu->addAction(name);
        m_logLevels.push_back(qMakePair(level, action));
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, [level, this](bool checked) {
            updateLogMenu(checked ? level : None , checked ? None : level);
        });
    }

private:
    int m_logLevelFlags = All;
    QVector<QPair<LogLevel, QAction*>> m_logLevels;
    QHash<qint64, QAction*> m_pids;
    QScopedPointer<QToolButton> m_filtersButton;
    QMenu *m_appsMenu;
    QList<CachedLine> m_cachedLines;
};

class ANDROID_EXPORT AndroidRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    AndroidRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;
    const QString remoteChannel() const;

protected:
    AndroidRunConfiguration(ProjectExplorer::Target *parent, AndroidRunConfiguration *source);
};

} // namespace Android
