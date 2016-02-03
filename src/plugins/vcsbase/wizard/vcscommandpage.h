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

#include <utils/shellcommandpage.h>

#include <QCoreApplication>

namespace VcsBase {
namespace Internal {

class VcsCommandPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
    Q_DECLARE_TR_FUNCTIONS(VcsBase::Internal::VcsCommandPage)

public:
    VcsCommandPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Core::Id typeId,
                              const QVariant &data) override;
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class VcsCommandPage : public Utils::ShellCommandPage
{
    Q_OBJECT

public:
    VcsCommandPage();

    void initializePage() override;

    void setCheckoutData(const QString &repo, const QString &baseDir, const QString &name,
                         const QStringList &args);
    void appendJob(bool skipEmpty, const QString &workDir, const QStringList &command,
                   const QVariant &condition, int timeoutFactor);
    void setVersionControlId(const QString &id);
    void setRunMessage(const QString &msg);

private slots:
    void delayedInitialize();

private:
    QString m_vcsId;
    QString m_repository;
    QString m_directory;
    QString m_name;
    QString m_runMessage;
    QStringList m_arguments;

    struct JobData {
        JobData(bool s, const QString &wd, const QStringList &c, const QVariant &cnd, int toF) :
            workDirectory(wd), job(c), condition(cnd), timeOutFactor(toF), skipEmptyArguments(s)
        { }

        QString workDirectory;
        QStringList job;
        QVariant condition;
        int timeOutFactor;
        bool skipEmptyArguments = false;
    };
    QList<JobData> m_additionalJobs;
};

} // namespace Internal
} // namespace VcsBase
