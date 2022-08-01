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

#include "../vcsbase_global.h"

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/filepath.h>
#include <utils/wizardpage.h>

#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLabel;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
class OutputFormatter;
}

namespace VcsBase {

class VcsCommand;

namespace Internal {

class VcsCommandPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
    Q_DECLARE_TR_FUNCTIONS(VcsBase::Internal::VcsCommandPage)

public:
    VcsCommandPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class ShellCommandPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    enum State { Idle, Running, Failed, Succeeded };

    explicit ShellCommandPage(QWidget *parent = nullptr);
    ~ShellCommandPage() override;

    void setStartedStatus(const QString &startedStatus);
    void start(VcsCommand *command);

    bool isComplete() const override;
    bool isRunning() const{ return m_state == Running; }

    void terminate();

    bool handleReject() override;

signals:
    void finished(bool success);

private:
    void slotFinished(bool success, const QVariant &cookie);

    QPlainTextEdit *m_logPlainTextEdit;
    Utils::OutputFormatter *m_formatter;
    QLabel *m_statusLabel;

    VcsCommand *m_command = nullptr;
    QString m_startedStatus;
    bool m_overwriteOutput = false;

    State m_state = Idle;
};

class VcsCommandPage : public ShellCommandPage
{
    Q_OBJECT

public:
    VcsCommandPage();

    void initializePage() override;

    void setCheckoutData(const QString &repo, const QString &baseDir, const QString &name,
                         const QStringList &args);
    void appendJob(bool skipEmpty, const Utils::FilePath &workDir, const QStringList &command,
                   const QVariant &condition, int timeoutFactor);
    void setVersionControlId(const QString &id);
    void setRunMessage(const QString &msg);

private slots:
    void delayedInitialize();

private:
    struct JobData
    {
        bool skipEmptyArguments = false;
        Utils::FilePath workDirectory;
        QStringList job;
        QVariant condition;
        int timeOutFactor;
    };

    QString m_vcsId;
    QString m_repository;
    QString m_directory;
    QString m_name;
    QString m_runMessage;
    QStringList m_arguments;
    QList<JobData> m_additionalJobs;
};

} // namespace Internal
} // namespace VcsBase
