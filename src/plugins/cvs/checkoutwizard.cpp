/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "checkoutwizard.h"
#include "checkoutwizardpage.h"
#include "cvsclient.h"
#include "cvsplugin.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/wizard/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

using namespace VcsBase;

namespace Cvs {
namespace Internal {

// --------------------------------------------------------------------
// CheckoutWizard:
// --------------------------------------------------------------------

CheckoutWizard::CheckoutWizard(const Utils::FileName &path, QWidget *parent) :
    BaseCheckoutWizard(Constants::VCS_ID_CVS, parent)
{
    auto cwp = new CheckoutWizardPage;
    cwp->setPath(path.toString());
    addPage(cwp);
}

VcsCommand *CheckoutWizard::createCommand(Utils::FileName *checkoutDir)
{
    // Collect parameters for the checkout command.
    // CVS does not allow for checking out into a different directory.
    const CheckoutWizardPage *cwp = find<CheckoutWizardPage>();
    QTC_ASSERT(cwp, return 0);

    *checkoutDir = Utils::FileName::fromString(cwp->path() + QLatin1Char('/') + cwp->repository());

    // cwp->repository() contains the CVS module to check out only.
    // The CVSROOT (== actual repository) for that module is part of the CVS settings.
    // The checkout will always go into a new subfolder named after the CVS module.

    return createCommandImpl(cwp->repository(), Utils::FileName::fromString(cwp->path()),
                             cwp->repository(), QStringList());
}

} // namespace Internal
} // namespace Cvs
