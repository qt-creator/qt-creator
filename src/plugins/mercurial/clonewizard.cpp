/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
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
#include "mercurialplugin.h"
#include "mercurialsettings.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>

using namespace Mercurial::Internal;
using namespace VcsBase;

CloneWizard::CloneWizard(QObject *parent)
    : BaseCheckoutWizard(parent),
    m_icon(QIcon(QLatin1String(":/mercurial/images/hg.png")))
{
    setId(QLatin1String(Constants::VCS_ID_MERCURIAL));
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

QList<QWizardPage *> CloneWizard::createParameterPages(const QString &path)
{
    QList<QWizardPage *> wizardPageList;
    const Core::IVersionControl *vc = MercurialPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        wizardPageList.append(new VcsConfigurationPage(vc));
    CloneWizardPage *page = new CloneWizardPage;
    page->setPath(path);
    wizardPageList.append(page);
    return wizardPageList;
}

QSharedPointer<AbstractCheckoutJob> CloneWizard::createJob(const QList<QWizardPage *> &parameterPages,
                                                           QString *checkoutPath)
{
    const CloneWizardPage *page = qobject_cast<const CloneWizardPage *>(parameterPages.front());

    if (!page)
        return QSharedPointer<AbstractCheckoutJob>();

    const MercurialSettings &settings = MercurialPlugin::instance()->settings();

    QString path = page->path();
    QString directory = page->directory();

    QStringList args;
    args << QLatin1String("clone") << page->repository() << directory;
    *checkoutPath = path + QLatin1Char('/') + directory;
    ProcessCheckoutJob *job = new ProcessCheckoutJob;
    job->addStep(settings.binaryPath(), args, path);
    return QSharedPointer<AbstractCheckoutJob>(job);
}
