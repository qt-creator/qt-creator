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

#pragma once

#include <utils/environment.h>

#include <QFutureInterface>
#include <QObject>

#include <qbs.h>

namespace QbsProjectManager {
namespace Internal {

class QbsProject;

class QbsProjectParser : public QObject
{
    Q_OBJECT

public:
    QbsProjectParser(QbsProjectManager::Internal::QbsProject *project,
                     QFutureInterface<bool> *fi);
    ~QbsProjectParser();

    void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir,
               const QString &configName);
    void startRuleExecution();
    void cancel();

    qbs::Project qbsProject() const;
    qbs::ErrorInfo error();

signals:
    void done(bool success);
    void ruleExecutionDone();

private:
    void handleQbsParsingDone(bool success);
    void handleQbsParsingProgress(int progress);
    void handleQbsParsingTaskSetup(const QString &description, int maximumProgressValue);

    QString pluginsBaseDirectory() const;
    QString resourcesBaseDirectory() const;
    QString libExecDirectory() const;

    void handleRuleExecutionDone();

    QString m_projectFilePath;
    qbs::SetupProjectJob *m_qbsSetupProjectJob;
    qbs::BuildJob *m_ruleExecutionJob;
    qbs::ErrorInfo m_error;
    qbs::Project m_project;
    bool m_dryRun;

    QFutureInterface<bool> *m_fi;
    int m_currentProgressBase;
};

} // namespace Internal
} // namespace QbsProjectManager
