// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/filepath.h>
#include <utils/wizardpage.h>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLabel;
QT_END_NAMESPACE

namespace Utils { class OutputFormatter; }

namespace VcsBase {

class VcsCommand;

namespace Internal {

class VcsCommandPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    VcsCommandPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                              const QVariant &data) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class VcsCommandPage : public Utils::WizardPage
{
public:
    VcsCommandPage();
    ~VcsCommandPage() override;

    void initializePage() override;
    bool isComplete() const override;
    bool handleReject() override;

    void setCheckoutData(const QString &repo, const QString &baseDir, const QString &name,
                         const QStringList &args);
    void appendJob(bool skipEmpty, const Utils::FilePath &workDir, const QStringList &command,
                   const QVariant &condition, int timeoutFactor);
    void setVersionControlId(const QString &id);
    void setRunMessage(const QString &msg);

private:
    void delayedInitialize();
    void start(VcsCommand *command);
    void finished(bool success);

    enum State { Idle, Running, Failed, Succeeded };

    struct JobData
    {
        bool skipEmptyArguments = false;
        Utils::FilePath workDirectory;
        QStringList job;
        QVariant condition;
        int timeOutFactor;
    };

    QPlainTextEdit *m_logPlainTextEdit = nullptr;
    Utils::OutputFormatter *m_formatter = nullptr;
    QLabel *m_statusLabel = nullptr;

    VcsCommand *m_command = nullptr;
    QString m_startedStatus;
    bool m_overwriteOutput = false;

    State m_state = Idle;
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
