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

namespace Git {

struct CloneWizardPagePrivate {
    CloneWizardPagePrivate();

    const QString mainLinePostfix;
    const QString gitPostFix;
    const QString protocolDelimiter;
};

CloneWizardPagePrivate::CloneWizardPagePrivate() :
    mainLinePostfix(QLatin1String("/mainline.git")),
    gitPostFix(QLatin1String(".git")),
    protocolDelimiter(QLatin1String("://"))
{
}

CloneWizardPage::CloneWizardPage(QWidget *parent) :
    VCSBase::BaseCheckoutWizardPage(parent),
    d(new CloneWizardPagePrivate)
{
    setSubTitle(tr("Specify repository URL, checkout directory and path."));
    setRepositoryLabel(tr("Clone URL:"));
}

CloneWizardPage::~CloneWizardPage()
{
    delete d;
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
     QStringList args = client->binary();
     const QString workingDirectory = path();
     const QString checkoutDir = directory();
     *checkoutPath = workingDirectory + QLatin1Char('/') + checkoutDir;
     args << QLatin1String("clone") << repository() << checkoutDir;
     const QString binary = args.front();
     args.pop_front();
     VCSBase::AbstractCheckoutJob *job = new VCSBase::ProcessCheckoutJob(binary, args, workingDirectory,
                                                                         client->processEnvironment());
     return QSharedPointer<VCSBase::AbstractCheckoutJob>(job);
}

} // namespace Git
