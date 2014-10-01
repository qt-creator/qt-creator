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

#include "basecheckoutwizard.h"
#include "basecheckoutwizardfactory.h"
#include "checkoutprogresswizardpage.h"

#include <coreplugin/basefilewizardfactory.h>

#include <utils/qtcassert.h>

#include <QPushButton>

/*!
    \class VcsBase::Internal::CheckoutWizardDialog

    Dialog used by \sa VcsBase::BaseCheckoutWizard. Overwrites reject() to first
    kill the checkout and then close.
 */

namespace VcsBase {

BaseCheckoutWizard::BaseCheckoutWizard(const Utils::FileName &path, QWidget *parent) :
    Utils::Wizard(parent),
    m_progressPage(new Internal::CheckoutProgressWizardPage),
    m_progressPageId(-1)
{
    Q_UNUSED(path);
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotPageChanged(int)));
    connect(m_progressPage, SIGNAL(terminated(bool)), this, SLOT(slotTerminated(bool)));
    setOption(QWizard::NoBackButtonOnLastPage);
}

void BaseCheckoutWizard::setTitle(const QString &title)
{
    m_progressPage->setTitle(title);
}

void BaseCheckoutWizard::setStartedStatus(const QString &title)
{
    m_progressPage->setStartedStatus(title);
}

void BaseCheckoutWizard::slotPageChanged(int id)
{
    if (id != m_progressPageId)
        return;

    VcsBase::Command *cmd = createCommand(&m_checkoutDir);
    QTC_ASSERT(cmd, done(QDialog::Rejected));

    // No "back" available while running.
    button(QWizard::BackButton)->setEnabled(false);
    m_progressPage->start(cmd);
}

void BaseCheckoutWizard::slotTerminated(bool success)
{
    // Allow to correct parameters
    if (!success)
        button(QWizard::BackButton)->setEnabled(true);
}

Utils::FileName BaseCheckoutWizard::run()
{
    m_progressPageId = addPage(m_progressPage);
    if (Utils::Wizard::exec() == QDialog::Accepted)
        return m_checkoutDir;
    else
        return Utils::FileName();
}

void BaseCheckoutWizard::reject()
{
    // First click kills, 2nd closes
    if (currentId() == m_progressPageId && m_progressPage->isRunning())
        m_progressPage->terminate();
    else
        QWizard::reject();
}

} // namespace VcsBase
