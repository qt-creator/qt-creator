#ifndef CLONEWIZARD_H
#define CLONEWIZARD_H

#include <vcsbase/basecheckoutwizard.h>
#include <vcsbase/checkoutjobs.h>

#include <QtGui/QIcon>

namespace Mercurial {
namespace Internal {

class CloneWizard : public VCSBase::BaseCheckoutWizard
{
public:
    CloneWizard(QObject *parent = 0);

    QIcon icon() const;
    QString description() const;
    QString name() const;

protected:
    QList<QWizardPage *> createParameterPages(const QString &path);
    QSharedPointer<VCSBase::AbstractCheckoutJob> createJob(const QList<QWizardPage *> &parameterPages,
                                                           QString *checkoutPath);

private:
    QIcon m_icon;
};

} //namespace Internal
} //namespace Mercurial

#endif // CLONEWIZARD_H
