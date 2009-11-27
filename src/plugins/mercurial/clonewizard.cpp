/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "clonewizard.h"
#include "clonewizardpage.h"
#include "mercurialplugin.h"
#include "mercurialsettings.h"

#include <vcsbase/checkoutjobs.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QDebug>

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

    const MercurialSettings &settings = MercurialPlugin::instance()->settings();

    QStringList args = settings.standardArguments();
    QString path = page->path();
    QString directory = page->directory();

    args << QLatin1String("clone") << page->repository() << directory;
    *checkoutPath = path + QLatin1Char('/') + directory;

    return QSharedPointer<VCSBase::AbstractCheckoutJob>(new VCSBase::ProcessCheckoutJob(settings.binary(),
                                                                                        args, path));
}
