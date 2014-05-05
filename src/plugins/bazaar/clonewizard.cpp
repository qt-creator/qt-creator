/**************************************************************************
**
** Copyright (c) 2014 Hugues Delorme
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clonewizard.h"
#include "clonewizardpage.h"
#include "cloneoptionspanel.h"
#include "bazaarplugin.h"
#include "bazaarclient.h"
#include "bazaarsettings.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/command.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>

#include <QDebug>

using namespace Bazaar::Internal;

CloneWizardFactory::CloneWizardFactory()
{
    setId(QLatin1String(VcsBase::Constants::VCS_ID_BAZAAR));
    setCustomLabels(tr("Cloning"), tr("Cloning started..."));
    setIcon(QIcon(QLatin1String(":/bazaar/images/bazaar.png")));
    setDescription(tr("Clones a Bazaar branch and tries to load the contained project."));
    setDisplayName(tr("Bazaar Clone (Or Branch)"));
}

VcsBase::BaseCheckoutWizard *CloneWizardFactory::create(const QList<QWizardPage *> &parameterPages, QWidget *parent) const
{
    return new CloneWizard(parameterPages, parent);
}

QList<QWizardPage *> CloneWizardFactory::createParameterPages(const QString &path)
{
    QList<QWizardPage *> wizardPageList;
    const Core::IVersionControl *vc = BazaarPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        wizardPageList.append(new VcsBase::VcsConfigurationPage(vc));
    CloneWizardPage *page = new CloneWizardPage;
    page->setPath(path);
    wizardPageList.append(page);
    return wizardPageList;
}

VcsBase::Command *CloneWizardFactory::createCommand(const QList<QWizardPage *> &parameterPages,
                                             QString *checkoutPath)
{
    const CloneWizardPage *page = qobject_cast<const CloneWizardPage *>(parameterPages.front());

    if (!page)
        return 0;

    const BazaarSettings &settings = BazaarPlugin::instance()->settings();
    *checkoutPath = page->path() + QLatin1Char('/') + page->directory();

    const CloneOptionsPanel *panel = page->cloneOptionsPanel();
    QStringList extraOptions;
    if (panel->isUseExistingDirectoryOptionEnabled())
        extraOptions += QLatin1String("--use-existing-dir");
    if (panel->isStackedOptionEnabled())
        extraOptions += QLatin1String("--stacked");
    if (panel->isStandAloneOptionEnabled())
        extraOptions += QLatin1String("--standalone");
    if (panel->isBindOptionEnabled())
        extraOptions += QLatin1String("--bind");
    if (panel->isSwitchOptionEnabled())
        extraOptions += QLatin1String("--switch");
    if (panel->isHardLinkOptionEnabled())
        extraOptions += QLatin1String("--hardlink");
    if (panel->isNoTreeOptionEnabled())
        extraOptions += QLatin1String("--no-tree");
    if (!panel->revision().isEmpty())
        extraOptions << QLatin1String("-r") << panel->revision();
    const BazaarClient *client = BazaarPlugin::instance()->client();
    QStringList args;
    args << client->vcsCommandString(BazaarClient::CloneCommand)
         << extraOptions << page->repository() << page->directory();

    VcsBase::Command *command = new VcsBase::Command(settings.binaryPath(), page->path(),
                                                     client->processEnvironment());
    command->addJob(args, -1);
    return command;
}

// --------------------------------------------------------------------
// CloneWizard:
// --------------------------------------------------------------------

CloneWizard::CloneWizard(const QList<QWizardPage *> &parameterPages, QWidget *parent) :
    VcsBase::BaseCheckoutWizard(parameterPages, parent)
{ }
