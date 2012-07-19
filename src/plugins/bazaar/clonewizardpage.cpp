/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Hugues Delorme
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
#include "cloneoptionspanel.h"

using namespace Bazaar::Internal;

CloneWizardPage::CloneWizardPage(QWidget *parent)
    : VcsBase::BaseCheckoutWizardPage(parent),
      m_optionsPanel(new CloneOptionsPanel)
{
    setTitle(tr("Location"));
    setSubTitle(tr("Specify repository URL, clone directory and path."));
    setRepositoryLabel(tr("Clone URL:"));
    setBranchSelectorVisible(false);
    addLocalControl(m_optionsPanel);
}

const CloneOptionsPanel *CloneWizardPage::cloneOptionsPanel() const
{
    return m_optionsPanel;
}

QString CloneWizardPage::directoryFromRepository(const QString &repository) const
{
    // Bazaar repositories are generally of the form
    // 'lp:project' or 'protocol://repositoryUrl/repository/'
    // We are just looking for repository.
    QString repo = repository.trimmed();
    if (repo.startsWith(QLatin1String("lp:")))
        return repo.mid(3);
    const QChar slash = QLatin1Char('/');
    if (repo.endsWith(slash))
        repo.truncate(repo.size() - 1);
    // Take the basename or the repository url
    return repo.mid(repo.lastIndexOf(slash) + 1);
}
