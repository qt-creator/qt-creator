#include "clonewizard.h"
#include "clonewizardpage.h"
#include "mercurialplugin.h"
#include "mercurialsettings.h"

#include <QtCore/QDebug>

using namespace Mercurial::Internal;

CloneWizard::CloneWizard(QObject *parent)
        :   VCSBase::BaseCheckoutWizard(parent),
        m_icon(QIcon(":/mercurial/images/hg.png"))
{
}

QIcon CloneWizard::icon() const
{
    return m_icon;
}

QString CloneWizard::description() const
{
    return tr("Clone a Mercurial repository");
}

QString CloneWizard::name() const
{
    return tr("Mercurial Clone");
}

QList<QWizardPage*> CloneWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage*> wizardPageList;
    CloneWizardPage *page = new CloneWizardPage;
    page->setPath(path);
    wizardPageList.push_back(page);
    return wizardPageList;
}

QSharedPointer<VCSBase::AbstractCheckoutJob> CloneWizard::createJob(const QList<QWizardPage *> &parameterPages,
                                                                    QString *checkoutPath)
{
    const CloneWizardPage *page = qobject_cast<const CloneWizardPage *>(parameterPages.front());

    if (!page)
        return QSharedPointer<VCSBase::AbstractCheckoutJob>();

    MercurialSettings *settings = MercurialPlugin::instance()->settings();

    QStringList args = settings->standardArguments();
    QString path = page->path();
    QString directory = page->directory();

    args << "clone" << page->repository() << directory;
    *checkoutPath = path + "/" + directory;

    return QSharedPointer<VCSBase::AbstractCheckoutJob>(new VCSBase::ProcessCheckoutJob(settings->binary(),
                                                                                        args, path));
}
