/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "clonewizard.h"
#include "clonewizardpage.h"
#include "mercurialplugin.h"
#include "mercurialsettings.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>

using namespace Mercurial::Internal;

CloneWizard::CloneWizard(QObject *parent)
        :   VCSBase::BaseCheckoutWizard(parent),
        m_icon(QIcon(QLatin1String(":/mercurial/images/hg.png")))
{
    setId(QLatin1String(VCSBase::Constants::VCS_ID_MERCURIAL));
}

QIcon CloneWizard::icon() const
{
    return m_icon;
}

QString CloneWizard::description() const
{
    return tr("Clones a Mercurial repository and tries to load the contained project.");
}

QString CloneWizard::displayName() const
{
    return tr("Mercurial Clone");
}

QList<QWizardPage*> CloneWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage*> wizardPageList;
    const Core::IVersionControl *vc = MercurialPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        wizardPageList.append(new VCSBase::VcsConfigurationPage(vc));
    CloneWizardPage *page = new CloneWizardPage;
    page->setPath(path);
    wizardPageList.append(page);
    return wizardPageList;
}

QSharedPointer<VCSBase::AbstractCheckoutJob> CloneWizard::createJob(const QList<QWizardPage *> &parameterPages,
                                                                    QString *checkoutPath)
{
    const CloneWizardPage *page = qobject_cast<const CloneWizardPage *>(parameterPages.front());

    if (!page)
        return QSharedPointer<VCSBase::AbstractCheckoutJob>();

    const MercurialSettings &settings = MercurialPlugin::instance()->settings();

    QString path = page->path();
    QString directory = page->directory();

    QStringList args;
    args << QLatin1String("clone") << page->repository() << directory;
    *checkoutPath = path + QLatin1Char('/') + directory;
    VCSBase::ProcessCheckoutJob *job = new VCSBase::ProcessCheckoutJob;
    job->addStep(settings.stringValue(MercurialSettings::binaryPathKey), args, path);
    return QSharedPointer<VCSBase::AbstractCheckoutJob>(job);
}
