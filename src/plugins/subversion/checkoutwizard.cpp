/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "checkoutwizard.h"
#include "checkoutwizardpage.h"
#include "subversionplugin.h"
#include "subversionclient.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/command.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

#include <QIcon>

namespace Subversion {
namespace Internal {

CheckoutWizardFactory::CheckoutWizardFactory()
{
    setId(QLatin1String(VcsBase::Constants::VCS_ID_SUBVERSION));
    setIcon(QIcon(QLatin1String(":/subversion/images/subversion.png")));
    setDescription(tr("Checks out a Subversion repository and tries to load the contained project."));
    setDisplayName(tr("Subversion Checkout"));
}

QList<QWizardPage*> CheckoutWizardFactory::createParameterPages(const QString &path)
{
    QList<QWizardPage*> rc;
    const Core::IVersionControl *vc = SubversionPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        rc.append(new VcsBase::VcsConfigurationPage(vc));
    CheckoutWizardPage *cwp = new CheckoutWizardPage;
    cwp->setPath(path);
    rc.append(cwp);
    return rc;
}

VcsBase::Command *CheckoutWizardFactory::createCommand(const QList<QWizardPage*> &parameterPages,
                                                QString *checkoutPath)
{
    // Collect parameters for the checkout command.
    const CheckoutWizardPage *cwp = qobject_cast<const CheckoutWizardPage *>(parameterPages.front());
    QTC_ASSERT(cwp, return 0);
    const SubversionSettings settings = SubversionPlugin::instance()->settings();
    const QString binary = settings.binaryPath();
    const QString directory = cwp->directory();
    QStringList args;
    args << QLatin1String("checkout") << cwp->repository() << directory;
    const QString workingDirectory = cwp->path();
    *checkoutPath = workingDirectory + QLatin1Char('/') + directory;
    if (settings.hasAuthentication()) {
        const QString user = settings.stringValue(SubversionSettings::userKey);
        const QString pwd = settings.stringValue(SubversionSettings::passwordKey);
        args = SubversionClient::addAuthenticationOptions(args, user, pwd);
    }
    VcsBase::Command *command = new VcsBase::Command(binary, workingDirectory,
                                                     QProcessEnvironment::systemEnvironment());
    command->addJob(args, -1);
    return command;
}

// --------------------------------------------------------------------
// CheckoutWizard:
// --------------------------------------------------------------------

CheckoutWizard::CheckoutWizard(const QList<QWizardPage *> &parameterPages, QWidget *parent) :
    VcsBase::BaseCheckoutWizard(parameterPages, parent)
{ }

} // namespace Internal
} // namespace Subversion
