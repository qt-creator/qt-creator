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
#include "subversionconstants.h"
#include "subversionplugin.h"
#include "subversionclient.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/wizard/vcsconfigurationpage.h>
#include <utils/qtcassert.h>

using namespace Utils;
using namespace VcsBase;

namespace Subversion {
namespace Internal {

// --------------------------------------------------------------------
// CheckoutWizard:
// --------------------------------------------------------------------

CheckoutWizard::CheckoutWizard(const FileName &path, QWidget *parent) :
    BaseCheckoutWizard(VcsBase::Constants::VCS_ID_SUBVERSION, parent)
{
    auto cwp = new CheckoutWizardPage;
    cwp->setPath(path.toString());
    addPage(cwp);
}

VcsCommand *CheckoutWizard::createCommand(FileName *checkoutDir)
{
    // Collect parameters for the checkout command.
    const CheckoutWizardPage *cwp = find<CheckoutWizardPage>();
    QTC_ASSERT(cwp, return 0);

    QStringList args;
    if (cwp->trustServerCert())
        args << QLatin1String("--trust-server-cert");
    const QString directory = cwp->directory();
    const QString workingDirectory = cwp->path();

    *checkoutDir = FileName::fromString(workingDirectory + QLatin1Char('/') + directory);

    return createCommandImpl(cwp->repository(), Utils::FileName::fromString(workingDirectory),
                             directory, args);
}

} // namespace Internal
} // namespace Subversion
