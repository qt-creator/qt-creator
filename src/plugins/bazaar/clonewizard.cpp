/**************************************************************************
**
** Copyright (C) 2015 Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/wizard/vcsconfigurationpage.h>

#include <utils/qtcassert.h>

using namespace VcsBase;

namespace Bazaar {
namespace Internal {

// --------------------------------------------------------------------
// CloneWizard:
// --------------------------------------------------------------------

CloneWizard::CloneWizard(const Utils::FileName &path, QWidget *parent) :
    BaseCheckoutWizard(Constants::VCS_ID_BAZAAR, parent)
{
    setTitle(tr("Cloning"));
    setStartedStatus(tr("Cloning started..."));

    auto page = new CloneWizardPage;
    page->setPath(path.toString());
    addPage(page);
}

VcsCommand *CloneWizard::createCommand(Utils::FileName *checkoutDir)
{
    const CloneWizardPage *cwp = find<CloneWizardPage>();
    QTC_ASSERT(cwp, return 0);

    *checkoutDir = Utils::FileName::fromString(cwp->path() + QLatin1Char('/') + cwp->directory());

    const CloneOptionsPanel *panel = cwp->cloneOptionsPanel();
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

    return createCommandImpl(cwp->repository(), Utils::FileName::fromString(cwp->path()),
                             cwp->directory(), extraOptions);
}

} // namespace Internal
} // namespace Bazaar
