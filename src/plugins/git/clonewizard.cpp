/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "clonewizard.h"
#include "clonewizardpage.h"

#include "gitplugin.h"
#include "gitversioncontrol.h"

#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

#include <QIcon>

namespace Git {
namespace Internal {

CloneWizard::CloneWizard(QObject *parent) :
        VcsBase::BaseCheckoutWizard(parent)
{
    setId(QLatin1String(VcsBase::Constants::VCS_ID_GIT));
}

QIcon CloneWizard::icon() const
{
    return QIcon(QLatin1String(":/git/images/git.png"));
}

QString CloneWizard::description() const
{
    return tr("Clones a Git repository and tries to load the contained project.");
}

QString CloneWizard::displayName() const
{
    return tr("Git Repository Clone");
}

QList<QWizardPage*> CloneWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage*> rc;
    const Internal::GitVersionControl *vc = Internal::GitPlugin::instance()->gitVersionControl();
    if (!vc->isConfigured())
        rc.append(new VcsBase::VcsConfigurationPage(vc));
    CloneWizardPage *cwp = new CloneWizardPage;
    cwp->setPath(path);
    rc.push_back(cwp);
    return rc;
}

QSharedPointer<VcsBase::AbstractCheckoutJob> CloneWizard::createJob(const QList<QWizardPage*> &parameterPages,
                                                                    QString *checkoutPath)
{
    // Collect parameters for the clone command.
    const CloneWizardPage *cwp = 0;
    foreach (QWizardPage *wp, parameterPages) {
        cwp = qobject_cast<const CloneWizardPage *>(wp);
        if (cwp)
            break;
    }

    QTC_ASSERT(cwp, return QSharedPointer<VcsBase::AbstractCheckoutJob>());
    return cwp->createCheckoutJob(checkoutPath);
}

} // namespace Internal
} // namespace Git
