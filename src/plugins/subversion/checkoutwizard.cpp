/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "checkoutwizard.h"
#include "checkoutwizardpage.h"
#include "subversionplugin.h"

#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QIcon>

namespace Subversion {
namespace Internal {

CheckoutWizard::CheckoutWizard(QObject *parent) :
        VCSBase::BaseCheckoutWizard(parent)
{
    setId(QLatin1String(VCSBase::Constants::VCS_ID_SUBVERSION));
}

QIcon CheckoutWizard::icon() const
{
    return QIcon(QLatin1String(":/subversion/images/subversion.png"));
}

QString CheckoutWizard::description() const
{
    return tr("Checks out a Subversion repository and tries to load the contained project.");
}

QString CheckoutWizard::displayName() const
{
    return tr("Subversion Checkout");
}

QList<QWizardPage*> CheckoutWizard::createParameterPages(const QString &path)
{
    CheckoutWizardPage *cwp = new CheckoutWizardPage;
    cwp->setPath(path);
    QList<QWizardPage*> rc;
    rc.push_back(cwp);
    return rc;
}

QSharedPointer<VCSBase::AbstractCheckoutJob> CheckoutWizard::createJob(const QList<QWizardPage*> &parameterPages,
                                                                    QString *checkoutPath)
{
    // Collect parameters for the checkout command.
    const CheckoutWizardPage *cwp = qobject_cast<const CheckoutWizardPage *>(parameterPages.front());
    QTC_ASSERT(cwp, return QSharedPointer<VCSBase::AbstractCheckoutJob>())
    const SubversionSettings settings = SubversionPlugin::subversionPluginInstance()->settings();
    const QString binary = settings.svnCommand;
    const QString directory = cwp->directory();
    QStringList args;
    args << QLatin1String("checkout") << cwp->repository() << directory;
    const QString workingDirectory = cwp->path();
    *checkoutPath = workingDirectory + QLatin1Char('/') + directory;
    const QStringList completeArgs = settings.hasAuthentication() ?
                                     SubversionPlugin::addAuthenticationOptions(args, settings.user, settings.password) :
                                     args;
    VCSBase::ProcessCheckoutJob *job = new VCSBase::ProcessCheckoutJob;
    job->addStep(binary, completeArgs, workingDirectory);
    return QSharedPointer<VCSBase::AbstractCheckoutJob>(job);
}

} // namespace Internal
} // namespace Subversion
