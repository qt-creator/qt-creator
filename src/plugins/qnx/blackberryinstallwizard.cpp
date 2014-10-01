/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryinstallwizard.h"
#include "blackberryinstallwizardpages.h"

#include <QAbstractButton>

#include <QMessageBox>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryInstallWizard::BlackBerryInstallWizard(BlackBerryInstallerDataHandler::Mode mode,
                                                 BlackBerryInstallerDataHandler::Target target,
                                                 const QString& version,
                                                 QWidget *parent)
    : Utils::Wizard(parent)
    , m_ndkPage(0)
    , m_targetPage(0)
{
    setWindowTitle(tr("BlackBerry NDK Installation Wizard"));

    m_data.mode = mode;
    m_data.installTarget = target;
    m_data.version = version;


    if (m_data.mode != BlackBerryInstallerDataHandler::UninstallMode) {
        m_optionPage = new BlackBerryInstallWizardOptionPage(m_data, this);
        m_ndkPage = new BlackBerryInstallWizardNdkPage(m_data, this);
        m_targetPage = new BlackBerryInstallWizardTargetPage(m_data, this);
        setPage(OptionPage, m_optionPage);
        setPage(NdkPageId, m_ndkPage);
        setPage(TargetPageId, m_targetPage);
    }

    m_processPage = new BlackBerryInstallWizardProcessPage(m_data, this);
    m_finalPage = new BlackBerryInstallWizardFinalPage(m_data, this);

    connect(m_finalPage, SIGNAL(done()), this, SIGNAL(processFinished()));
    disconnect(button(CancelButton), SIGNAL(clicked()), this, SLOT(reject()));
    connect(button(CancelButton), SIGNAL(clicked()), this, SLOT(handleProcessCancelled()));

    setPage(ProcessPageId, m_processPage);
    setPage(FinalPageId, m_finalPage);

    m_finalPage->setCommitPage(true);

    setOption(DisabledBackButtonOnLastPage, true);
}

void BlackBerryInstallWizard::handleProcessCancelled()
{
    if ((m_targetPage && m_targetPage->isProcessRunning())
         || m_processPage->isProcessRunning()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Confirmation"),
                                                                         tr("Are you sure you want to cancel?"),
                                                                         QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::No)
            return;
    }

    reject();
}
