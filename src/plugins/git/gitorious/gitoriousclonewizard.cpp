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

#include "gitoriousclonewizard.h"
#include "gitorioushostwizardpage.h"
#include "gitoriousprojectwizardpage.h"
#include "gitoriousrepositorywizardpage.h"

#include "../clonewizardpage.h"
#include "../gitplugin.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

#include <QUrl>
#include <QIcon>

namespace Gitorious {
namespace Internal {

//  GitoriousCloneWizardPage: A git clone page taking its URL from the
//  projects page.

class GitoriousCloneWizardPage : public Git::CloneWizardPage {
public:
    explicit GitoriousCloneWizardPage(const GitoriousRepositoryWizardPage *rp, QWidget *parent = 0);
    void initializePage();

private:
    const GitoriousRepositoryWizardPage *m_repositoryPage;
};

GitoriousCloneWizardPage::GitoriousCloneWizardPage(const GitoriousRepositoryWizardPage *rp, QWidget *parent) :
    Git::CloneWizardPage(parent),
    m_repositoryPage(rp)
{
}

void GitoriousCloneWizardPage::initializePage()
{
    setRepository(m_repositoryPage->repositoryURL().toString());
}

// -------- GitoriousCloneWizard
GitoriousCloneWizard::GitoriousCloneWizard(QObject *parent) :
        VcsBase::BaseCheckoutWizard(parent)
{
    setId(QLatin1String(VcsBase::Constants::VCS_ID_GIT));
}

QIcon GitoriousCloneWizard::icon() const
{
    return QIcon(QLatin1String(":/git/images/gitorious.png"));
}

QString GitoriousCloneWizard::description() const
{
    return tr("Clones a Gitorious repository and tries to load the contained project.");
}

QString GitoriousCloneWizard::displayName() const
{
    return tr("Gitorious Repository Clone");
}

QList<QWizardPage*> GitoriousCloneWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage*> rc;
    const Core::IVersionControl *vc = Git::Internal::GitPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        rc.append(new VcsBase::VcsConfigurationPage(vc));

    GitoriousHostWizardPage *hostPage = new GitoriousHostWizardPage;
    GitoriousProjectWizardPage *projectPage = new GitoriousProjectWizardPage(hostPage);
    GitoriousRepositoryWizardPage *repoPage = new GitoriousRepositoryWizardPage(projectPage);
    GitoriousCloneWizardPage *clonePage = new GitoriousCloneWizardPage(repoPage);
    clonePage->setPath(path);

    rc << hostPage << projectPage << repoPage << clonePage;
    return rc;
}

QSharedPointer<VcsBase::AbstractCheckoutJob> GitoriousCloneWizard::createJob(const QList<QWizardPage*> &parameterPages,
                                                                    QString *checkoutPath)
{
    const Git::CloneWizardPage *cwp = qobject_cast<const Git::CloneWizardPage *>(parameterPages.back());
    QTC_ASSERT(cwp, return QSharedPointer<VcsBase::AbstractCheckoutJob>());
    return cwp->createCheckoutJob(checkoutPath);
}

} // namespace Internal
} // namespace Gitorius
