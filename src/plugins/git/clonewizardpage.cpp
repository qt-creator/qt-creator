/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "clonewizardpage.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <vcsbase/checkoutjobs.h>
#include <utils/qtcassert.h>

#include <QtGui/QCheckBox>

namespace Git {

struct CloneWizardPagePrivate {
    CloneWizardPagePrivate();

    const QString mainLinePostfix;
    const QString gitPostFix;
    const QString protocolDelimiter;
    QCheckBox *deleteMasterCheckBox;
};

CloneWizardPagePrivate::CloneWizardPagePrivate() :
    mainLinePostfix(QLatin1String("/mainline.git")),
    gitPostFix(QLatin1String(".git")),
    protocolDelimiter(QLatin1String("://")),
    deleteMasterCheckBox(0)
{
}

CloneWizardPage::CloneWizardPage(QWidget *parent) :
    VCSBase::BaseCheckoutWizardPage(parent),
    d(new CloneWizardPagePrivate)
{
    setTitle(tr("Location"));
    setSubTitle(tr("Specify repository URL, checkout directory and path."));
    setRepositoryLabel(tr("Clone URL:"));
    d->deleteMasterCheckBox = new QCheckBox(tr("Delete master branch"));
    addControl(d->deleteMasterCheckBox);
    setDeleteMasterBranch(true);
}

CloneWizardPage::~CloneWizardPage()
{
    delete d;
}

bool CloneWizardPage::deleteMasterBranch() const
{
    return d->deleteMasterCheckBox->isChecked();
}

void CloneWizardPage::setDeleteMasterBranch(bool v)
{
    d->deleteMasterCheckBox->setChecked(v);
}

QString CloneWizardPage::directoryFromRepository(const QString &urlIn) const
{
    /* Try to figure out a good directory name from something like:
     * 'user@host:qt/qt.git', 'http://host/qt/qt.git' 'local repo'
     * ------> 'qt' .  */

    QString url = urlIn.trimmed();
    const QChar slash = QLatin1Char('/');
    // remove host
    const int protocolDelimiterPos = url.indexOf(d->protocolDelimiter); // "://"
    const int startRepoSearchPos = protocolDelimiterPos == -1 ? 0 : protocolDelimiterPos + d->protocolDelimiter.size();
    int repoPos = url.indexOf(QLatin1Char(':'), startRepoSearchPos);
    if (repoPos == -1)
        repoPos = url.indexOf(slash, startRepoSearchPos);
    if (repoPos != -1)
        url.remove(0, repoPos + 1);
    // Remove postfixes
    if (url.endsWith(d->mainLinePostfix)) {
        url.truncate(url.size() - d->mainLinePostfix.size());
    } else {
        if (url.endsWith(d->gitPostFix)) {
            url.truncate(url.size() - d->gitPostFix.size());
        }
    }
    // Check for equal parts, something like "qt/qt" -> "qt"
    const int slashPos = url.indexOf(slash);
    if (slashPos != -1 && slashPos == (url.size() - 1) / 2) {
        if (url.leftRef(slashPos) == url.rightRef(slashPos))
            url.truncate(slashPos);
    }
    // fix invalid characters
    const QChar dash = QLatin1Char('-');
    url.replace(slash, dash);
    url.replace(QLatin1Char('.'), dash);
    return url;
}

QSharedPointer<VCSBase::AbstractCheckoutJob> CloneWizardPage::createCheckoutJob(QString *checkoutPath) const
{
     const Internal::GitClient *client = Internal::GitPlugin::instance()->gitClient();
     const QString workingDirectory = path();
     const QString checkoutDir = directory();
     *checkoutPath = workingDirectory + QLatin1Char('/') + checkoutDir;

     QStringList baseArgs = client->binary();
     const QString binary = baseArgs.front();
     baseArgs.pop_front();

     QStringList args;
     args << QLatin1String("clone") << repository() << checkoutDir;

     VCSBase::ProcessCheckoutJob *job = new VCSBase::ProcessCheckoutJob;
     const QProcessEnvironment env = client->processEnvironment();
     // 1) Basic checkout step
     job->addStep(binary, baseArgs + args, workingDirectory, env);
     const QString checkoutBranch = branch();

     // 2) Checkout branch, change to checkoutDir
     const QString masterBranch = QLatin1String("master");
     if (!checkoutBranch.isEmpty() && checkoutBranch != masterBranch) {
         // Create branch
         args.clear();
         args << QLatin1String("branch") << QLatin1String("--track")
                 << checkoutBranch << (QLatin1String("origin/")  + checkoutBranch);
         job->addStep(binary, baseArgs + args, *checkoutPath, env);
         // Checkout branch
         args.clear();
         args << QLatin1String("checkout") << checkoutBranch;
         job->addStep(binary, baseArgs + args, *checkoutPath, env);
         // Delete master if desired
         if (deleteMasterBranch()) {
             args.clear();
             args << QLatin1String("branch") << QLatin1String("-D") << masterBranch;
             job->addStep(binary, baseArgs + args, *checkoutPath, env);
         }
     }

     return QSharedPointer<VCSBase::AbstractCheckoutJob>(job);
}

QStringList CloneWizardPage::branches(const QString &repository, int *current)
{
    // Run git on remote repository if URL is complete
    *current = 0;
    if (!repository.endsWith(d->gitPostFix))
        return QStringList();
     const QStringList branches = Internal::GitPlugin::instance()->gitClient()->synchronousRepositoryBranches(repository);
     *current = branches.indexOf(QLatin1String("master"));
     return branches;
}

} // namespace Git
