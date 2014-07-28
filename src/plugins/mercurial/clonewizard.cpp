/**************************************************************************
**
** Copyright (c) 2014 Brian McGillion
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
#include <vcsbase/command.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>

using namespace Mercurial::Internal;
using namespace VcsBase;

// --------------------------------------------------------------------
// CloneWizard:
// --------------------------------------------------------------------

CloneWizard::CloneWizard(const Utils::FileName &path, QWidget *parent) :
    VcsBase::BaseCheckoutWizard(path, parent)
{
    setTitle(tr("Cloning"));
    setStartedStatus(tr("Cloning started..."));

    const Core::IVersionControl *vc = MercurialPlugin::instance()->versionControl();
    if (!vc->isConfigured())
        addPage(new VcsConfigurationPage(vc));
    CloneWizardPage *page = new CloneWizardPage;
    page->setPath(path.toString());
    addPage(page);
}

Command *CloneWizard::createCommand(Utils::FileName *checkoutDir)
{
    const CloneWizardPage *cwp = 0;
    foreach (int pageId, pageIds()) {
        if ((cwp = qobject_cast<const CloneWizardPage *>(page(pageId))))
            break;
    }

    if (!cwp)
        return 0;

    const MercurialSettings &settings = MercurialPlugin::settings();

    QString path = cwp->path();
    QString directory = cwp->directory();

    QStringList args;
    args << QLatin1String("clone") << cwp->repository() << directory;
    *checkoutDir = Utils::FileName::fromString(path + QLatin1Char('/') + directory);
    VcsBase::Command *command = new VcsBase::Command(settings.binaryPath(), path,
                                                     QProcessEnvironment::systemEnvironment());
    command->addJob(args, -1);
    return command;
}
