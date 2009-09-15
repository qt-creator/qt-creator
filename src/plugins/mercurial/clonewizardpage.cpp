#include "clonewizardpage.h"

using namespace Mercurial::Internal;

CloneWizardPage::CloneWizardPage(QWidget *parent)
        : VCSBase::BaseCheckoutWizardPage(parent)
{
    setRepositoryLabel("Clone URL:");
}

QString CloneWizardPage::directoryFromRepository(const QString &repository) const
{
    //mercruial repositories are generally of the form protocol://repositoryUrl/repository/
    //we are just looking for repository.
    QString repo = repository.trimmed();
    if (repo.endsWith('/'))
        repo = repo.remove(-1, 1);

    //Take the basename or the repository url
    return repo.mid(repo.lastIndexOf('/') + 1);
}
