/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QBSSTEP_H
#define QBSSTEP_H

#include "qbsbuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

#include <qbs.h>

namespace QbsProjectManager {
namespace Internal {

class QbsStepConfigWidget;

class QbsStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    ~QbsStep();

    bool init();

    void run(QFutureInterface<bool> &fi);

    QFutureInterface<bool> *future() const;

    bool runInGuiThread() const;
    void cancel();

    bool dryRun() const;
    bool keepGoing() const;
    int maxJobs() const;
    QString buildVariant() const;

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

signals:
    void qbsBuildOptionsChanged();

private slots:
    virtual void jobDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

protected:
    QbsStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    QbsStep(ProjectExplorer::BuildStepList *bsl, const QbsStep *other);

    QbsBuildConfiguration *currentBuildConfiguration() const;

    virtual qbs::AbstractJob *createJob() = 0;

    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    qbs::AbstractJob *job() const { return m_job; }
    qbs::BuildOptions buildOptions() const { return m_qbsBuildOptions; }

private:
    void setDryRun(bool dr);
    void setKeepGoing(bool kg);
    void setMaxJobs(int jobcount);

    qbs::BuildOptions m_qbsBuildOptions;

    QFutureInterface<bool> *m_fi;
    qbs::AbstractJob *m_job;
    int m_progressBase;

    friend class QbsStepConfigWidget;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSSTEP_H
