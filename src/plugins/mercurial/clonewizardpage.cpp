/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "clonewizardpage.h"

using namespace Mercurial::Internal;

CloneWizardPage::CloneWizardPage(QWidget *parent)
        : VcsBase::BaseCheckoutWizardPage(parent)
{
    setTitle(tr("Location"));
    setSubTitle(tr("Specify repository URL, checkout directory and path."));
    setRepositoryLabel(tr("Clone URL:"));
    setBranchSelectorVisible(false);
}

QString CloneWizardPage::directoryFromRepository(const QString &repository) const
{
    // Mercurial repositories are generally of the form protocol://repositoryUrl/repository/
    // We are just looking for repository.
    const QChar slash = QLatin1Char('/');
    QString repo = repository.trimmed();
    if (repo.endsWith(slash))
        repo.truncate(repo.size() - 1);

    // Take the basename or the repository url.
    return repo.mid(repo.lastIndexOf(slash) + 1);
}
