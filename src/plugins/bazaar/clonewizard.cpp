/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Hugues Delorme
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "cloneoptionspanel.h"
#include "bazaarplugin.h"
#include "bazaarclient.h"
#include "bazaarsettings.h"

#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QDebug>

using namespace Bazaar::Internal;

CloneWizard::CloneWizard(QObject *parent)
        :   VCSBase::BaseCheckoutWizard(parent),
        m_icon(QIcon(QLatin1String(":/bazaar/images/bazaar.png")))
{
    setId(QLatin1String(VCSBase::Constants::VCS_ID_BAZAAR));
}

QIcon CloneWizard::icon() const
{
    return m_icon;
}

QString CloneWizard::description() const
{
    return tr("Clones a Bazaar branch and tries to load the contained project.");
}

QString CloneWizard::displayName() const
{
    return tr("Bazaar Clone (Or Branch)");
}

QList<QWizardPage*> CloneWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage*> wizardPageList;
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

    const BazaarSettings &settings = BazaarPlugin::instance()->settings();
    QStringList args = settings.standardArguments();
    *checkoutPath = page->path() + QLatin1Char('/') + page->directory();

    const CloneOptionsPanel *panel = page->cloneOptionsPanel();
    BazaarClient::ExtraCommandOptions extraOptions;
    extraOptions[BazaarClient::UseExistingDirCloneOptionId] = panel->isUseExistingDirectoryOptionEnabled();
    extraOptions[BazaarClient::StackedCloneOptionId] = panel->isStackedOptionEnabled();
    extraOptions[BazaarClient::StandAloneCloneOptionId] = panel->isStandAloneOptionEnabled();
    extraOptions[BazaarClient::BindCloneOptionId] = panel->isBindOptionEnabled();
    extraOptions[BazaarClient::SwitchCloneOptionId] = panel->isSwitchOptionEnabled();
    extraOptions[BazaarClient::HardLinkCloneOptionId] = panel->isHardLinkOptionEnabled();
    extraOptions[BazaarClient::NoTreeCloneOptionId] = panel->isNoTreeOptionEnabled();
    extraOptions[BazaarClient::RevisionCloneOptionId] = panel->revision();
    const BazaarClient *client = BazaarPlugin::instance()->client();
    args << client->vcsCommandString(BazaarClient::CloneCommand)
         << client->cloneArguments(page->repository(), page->directory(), extraOptions);

    VCSBase::ProcessCheckoutJob *job = new VCSBase::ProcessCheckoutJob;
    job->addStep(settings.binary(), args, page->path());
    return QSharedPointer<VCSBase::AbstractCheckoutJob>(job);
}
