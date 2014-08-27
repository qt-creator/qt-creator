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

#include "clonewizard.h"
#include "clonewizardpage.h"

#include "gitplugin.h"
#include "gitversioncontrol.h"

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

using namespace VcsBase;

namespace Git {
namespace Internal {

// --------------------------------------------------------------------
// CloneWizard:
// --------------------------------------------------------------------

CloneWizard::CloneWizard(const Utils::FileName &path, QWidget *parent) :
    BaseCheckoutWizard(path, parent)
{
    setTitle(tr("Cloning"));
    setStartedStatus(tr("Cloning started..."));

    const Internal::GitVersionControl *vc = Internal::GitPlugin::instance()->gitVersionControl();
    if (!vc->isConfigured())
        addPage(new VcsConfigurationPage(vc));
    CloneWizardPage *cwp = new CloneWizardPage;
    cwp->setPath(path.toString());
    addPage(cwp);
}

VcsCommand *CloneWizard::createCommand(Utils::FileName *checkoutDir)
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
