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

#include "clonewizardpage.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <vcsbase/vcscommand.h>

#include <QCheckBox>

using namespace VcsBase;

namespace Git {

struct CloneWizardPagePrivate
{
    CloneWizardPagePrivate();

    bool urlIsLocal(const QString &url);

    const QString mainLinePostfix;
    const QString gitPostFix;
    QCheckBox *recursiveCheckBox;
};

CloneWizardPagePrivate::CloneWizardPagePrivate() :
    mainLinePostfix(QLatin1String("/mainline.git")),
    gitPostFix(QLatin1String(".git")),
    recursiveCheckBox(0)
{
}

bool CloneWizardPagePrivate::urlIsLocal(const QString &url)
{
    if (url.startsWith(QLatin1String("file://"))
        || url.startsWith(QLatin1Char('/'))
        || (url.at(0).isLetter() && url.at(1) == QLatin1Char(':') && url.at(2) == QLatin1Char('\\')))
        return true;
    return false;
}

CloneWizardPage::CloneWizardPage(QWidget *parent) :
    BaseCheckoutWizardPage(parent),
    d(new CloneWizardPagePrivate)
{
    setTitle(tr("Location"));
    setSubTitle(tr("Specify repository URL, checkout directory and path."));
    setRepositoryLabel(tr("Clone URL:"));
    d->recursiveCheckBox = new QCheckBox(tr("Recursive"));
    addLocalControl(d->recursiveCheckBox);
}

CloneWizardPage::~CloneWizardPage()
{
    delete d;
}

QString CloneWizardPage::directoryFromRepository(const QString &urlIn) const
{
    const QChar slash = QLatin1Char('/');
    QString url = urlIn.trimmed().replace(QLatin1Char('\\'), slash);

    // Remove postfixes
    if (url.endsWith(d->mainLinePostfix))
        url.truncate(url.size() - d->mainLinePostfix.size());
    else if (url.endsWith(d->gitPostFix))
        url.truncate(url.size() - d->gitPostFix.size());

    // extract repository name (last part of path)
    int startOfRepoName = url.lastIndexOf(slash);
    if (startOfRepoName == -1)
        startOfRepoName = url.lastIndexOf(QLatin1Char(':'));
    url.remove(0, startOfRepoName);

    // fix invalid characters
    const QChar dash = QLatin1Char('-');
    url.replace(QRegExp(QLatin1String("[^0-9a-zA-Z_.-]")), dash);
    // trim leading dashes (they are annoying and get created when using local pathes)
    url.replace(QRegExp(QLatin1String("^-+")), QString());
    return url;
}

VcsCommand *CloneWizardPage::createCheckoutJob(Utils::FileName *checkoutPath) const
{
     const Internal::GitClient *client = Internal::GitPlugin::instance()->gitClient();
     const QString workingDirectory = path();
     const QString checkoutDir = directory();
     *checkoutPath = Utils::FileName::fromString(workingDirectory + QLatin1Char('/') + checkoutDir);

     const QString checkoutBranch = branch();

     QStringList args(QLatin1String("clone"));
     if (!checkoutBranch.isEmpty())
         args << QLatin1String("--branch") << checkoutBranch;
     if (d->recursiveCheckBox->isChecked())
         args << QLatin1String("--recursive");
     args << QLatin1String("--progress") << repository() << checkoutDir;
     auto command = new VcsCommand(client->vcsBinary(), workingDirectory,
                                   client->processEnvironment());
     command->addFlags(VcsBasePlugin::MergeOutputChannels);
     command->addJob(args, -1);
     return command;
}

QStringList CloneWizardPage::branches(const QString &repository, int *current)
{
    // Run git on remote repository if an URL was specified.
    *current = -1;

    if (repository.isEmpty())
        return QStringList();
     const QStringList branches = Internal::GitPlugin::instance()->gitClient()->synchronousRepositoryBranches(repository);
     if (!branches.isEmpty())
         *current = 0; // default branch is always returned first!
     return branches;
}

} // namespace Git

#ifdef WITH_TESTS
#include <QTest>

void Git::CloneWizardPage::testDirectoryFromRepository()
{
    QFETCH(QString, repository);
    QFETCH(QString, localDirectory);

    QCOMPARE(directoryFromRepository(repository), localDirectory);
}
#endif
