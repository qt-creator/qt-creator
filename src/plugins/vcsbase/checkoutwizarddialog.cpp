/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "checkoutwizarddialog.h"
#include "basecheckoutwizard.h"
#include "checkoutjobs.h"
#include "checkoutprogresswizardpage.h"

#include <coreplugin/basefilewizard.h>

#include <QPushButton>

/*!
    \class VcsBase::Internal::CheckoutWizardDialog

    Dialog used by \sa VcsBase::BaseCheckoutWizard. Overwrites reject() to first
    kill the checkout and then close.
 */

namespace VcsBase {
namespace Internal {

CheckoutWizardDialog::CheckoutWizardDialog(const QList<QWizardPage *> &parameterPages,
                                           QWidget *parent) :
    Utils::Wizard(parent),
    m_progressPage(new CheckoutProgressWizardPage),
    m_progressPageId(-1)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    foreach (QWizardPage *wp, parameterPages)
        addPage(wp);
    m_progressPageId = parameterPages.size();
    setPage(m_progressPageId, m_progressPage);
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotPageChanged(int)));
    connect(m_progressPage, SIGNAL(terminated(bool)), this, SLOT(slotTerminated(bool)));
    Core::BaseFileWizard::setupWizard(this);
}

void CheckoutWizardDialog::slotPageChanged(int id)
{
    if (id == m_progressPageId)
        emit progressPageShown();
}

void CheckoutWizardDialog::slotTerminated(bool success)
{
    // Allow to correct parameters
    if (!success)
        button(QWizard::BackButton)->setEnabled(true);
}

void CheckoutWizardDialog::start(const QSharedPointer<AbstractCheckoutJob> &job)
{
    // No "back" available while running.
    button(QWizard::BackButton)->setEnabled(false);
    m_progressPage->start(job);
}

void CheckoutWizardDialog::reject()
{
    // First click kills, 2nd closes
    if (currentId() == m_progressPageId && m_progressPage->isRunning())
        m_progressPage->terminate();
    else
        QWizard::reject();
}

} // namespace Internal
} // namespace VcsBase
