/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSCOMMANDPAGE_H
#define VCSCOMMANDPAGE_H

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
                              const QVariant &data);
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage);
};

class VcsCommandPage : public Utils::ShellCommandPage
{
    Q_OBJECT

public:
    VcsCommandPage();

    void initializePage();

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

#endif // VCSCOMMANDPAGE_H
