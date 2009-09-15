#ifndef CLONEWIZARDPAGE_H
#define CLONEWIZARDPAGE_H

#include <vcsbase/basecheckoutwizardpage.h>

namespace Mercurial {
namespace Internal {

class CloneWizardPage : public VCSBase::BaseCheckoutWizardPage
{
    Q_OBJECT
public:
    CloneWizardPage(QWidget *parent = 0);

protected:
    QString directoryFromRepository(const QString &rrepository) const;
};

} //namespace Internal
} //namespace Mercurial

#endif // CLONEWIZARDPAGE_H
