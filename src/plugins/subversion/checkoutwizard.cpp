/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
    return QIcon();
}

QString CheckoutWizard::description() const
{
    return tr("Checks out a project from a Subversion repository.");
}

QString CheckoutWizard::name() const
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
    VCSBase::AbstractCheckoutJob *job = new VCSBase::ProcessCheckoutJob(binary, settings.addOptions(args),
                                                                        workingDirectory);
    return QSharedPointer<VCSBase::AbstractCheckoutJob>(job);
}

} // namespace Internal
} // namespace Subversion
