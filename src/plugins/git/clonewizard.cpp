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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clonewizard.h"
#include "clonewizardpage.h"

#include "gitplugin.h"
#include "gitversioncontrol.h"

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

namespace Git {
namespace Internal {

CloneWizardFactory::CloneWizardFactory()
{
    setId(QLatin1String(VcsBase::Constants::VCS_ID_GIT));
    setIcon(QIcon(QLatin1String(":/git/images/git.png")));
    setDescription(tr("Clones a Git repository and tries to load the contained project."));
    setDisplayName(tr("Git Repository Clone"));
}

VcsBase::BaseCheckoutWizard *CloneWizardFactory::create(const Utils::FileName &path, QWidget *parent) const
{
    return new CloneWizard(path, parent);
}

// --------------------------------------------------------------------
// CloneWizard:
// --------------------------------------------------------------------

CloneWizard::CloneWizard(const Utils::FileName &path, QWidget *parent) :
    VcsBase::BaseCheckoutWizard(path, parent)
{
    setTitle(tr("Cloning"));
    setStartedStatus(tr("Cloning started..."));

    const Internal::GitVersionControl *vc = Internal::GitPlugin::instance()->gitVersionControl();
    if (!vc->isConfigured())
        addPage(new VcsBase::VcsConfigurationPage(vc));
    CloneWizardPage *cwp = new CloneWizardPage;
    cwp->setPath(path.toString());
    addPage(cwp);
}

VcsBase::Command *CloneWizard::createCommand(Utils::FileName *checkoutDir)
{
    // Collect parameters for the clone command.
    const CloneWizardPage *cwp = 0;
    foreach (int pageId, pageIds()) {
        if ((cwp = qobject_cast<const CloneWizardPage *>(page(pageId))))
            break;
    }

    QTC_ASSERT(cwp, return 0);
    return cwp->createCheckoutJob(checkoutDir);
}

} // namespace Internal
} // namespace Git
