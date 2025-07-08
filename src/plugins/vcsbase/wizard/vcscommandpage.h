// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/filepath.h>
#include <utils/wizardpage.h>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLabel;
QT_END_NAMESPACE

namespace Utils { class OutputFormatter; }

namespace VcsBase::Internal {

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
    void setVersionControlId(const QString &id);
    void setRunMessage(const QString &msg);

private:
    void delayedInitialize();

    QPlainTextEdit *m_logPlainTextEdit = nullptr;
    Utils::OutputFormatter *m_formatter = nullptr;
    QLabel *m_statusLabel = nullptr;

    QString m_startedStatus;
    bool m_overwriteOutput = false;

    bool m_isComplete = false;
    QString m_vcsId;
    QString m_repository;
    QString m_directory;
    QString m_name;
    QString m_runMessage;
    QStringList m_arguments;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // namespace VcsBase::Internal
