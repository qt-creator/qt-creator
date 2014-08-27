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

#include "checkoutprogresswizardpage.h"
#include "vcscommand.h"
#include "vcsbaseplugin.h"

#include <utils/outputformatter.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

/*!
    \class VcsBase::Internal::CheckoutProgressWizardPage

    \brief The CheckoutProgressWizardPage implements a page showing the
    progress of an initial project checkout.

    Turns complete when the job succeeds.

    \sa VcsBase::BaseCheckoutWizard
*/

namespace VcsBase {
namespace Internal {

CheckoutProgressWizardPage::CheckoutProgressWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_startedStatus(tr("Checkout started...")),
    m_overwriteOutput(false),
    m_state(Idle)
{
    resize(264, 200);
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    m_logPlainTextEdit = new QPlainTextEdit;
    m_formatter = new Utils::OutputFormatter;
    m_logPlainTextEdit->setReadOnly(true);
    m_formatter->setPlainTextEdit(m_logPlainTextEdit);

    verticalLayout->addWidget(m_logPlainTextEdit);

    m_statusLabel = new QLabel;
    verticalLayout->addWidget(m_statusLabel);
    setTitle(tr("Checkout"));
}

CheckoutProgressWizardPage::~CheckoutProgressWizardPage()
{
    if (m_state == Running) // Paranoia!
        QApplication::restoreOverrideCursor();
    delete m_formatter;
}

void CheckoutProgressWizardPage::setStartedStatus(const QString &startedStatus)
{
    m_startedStatus = startedStatus;
}

void CheckoutProgressWizardPage::start(VcsCommand *command)
{
    if (!command) {
        m_logPlainTextEdit->setPlainText(tr("No job running, please abort."));
        return;
    }

    QTC_ASSERT(m_state != Running, return);
    m_command = command;
    command->setProgressiveOutput(true);
    connect(command, SIGNAL(output(QString)), this, SLOT(slotOutput(QString)));
    connect(command, SIGNAL(finished(bool,int,QVariant)), this, SLOT(slotFinished(bool,int,QVariant)));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_logPlainTextEdit->clear();
    m_overwriteOutput = false;
    m_statusLabel->setText(m_startedStatus);
    m_statusLabel->setPalette(QPalette());
    m_state = Running;
    command->execute();
}

void CheckoutProgressWizardPage::slotFinished(bool ok, int exitCode, const QVariant &)
{
    if (ok && exitCode == 0) {
        if (m_state == Running) {
            m_state = Succeeded;
            QApplication::restoreOverrideCursor();
            m_statusLabel->setText(tr("Succeeded."));
            QPalette palette;
            palette.setColor(QPalette::Active, QPalette::Text, Qt::green);
            m_statusLabel->setPalette(palette);
            emit completeChanged();
            emit terminated(true);
        }
    } else {
        m_logPlainTextEdit->appendPlainText(m_error);
        if (m_state == Running) {
            m_state = Failed;
            QApplication::restoreOverrideCursor();
            m_statusLabel->setText(tr("Failed."));
            QPalette palette;
            palette.setColor(QPalette::Active, QPalette::Text, Qt::red);
            m_statusLabel->setPalette(palette);
            emit terminated(false);
        }
    }
}

void CheckoutProgressWizardPage::slotOutput(const QString &text)
{
    m_formatter->appendMessage(text, Utils::StdOutFormat);
}

void CheckoutProgressWizardPage::slotError(const QString &text)
{
    m_error.append(text);
}

void CheckoutProgressWizardPage::terminate()
{
    if (m_command)
        m_command->cancel();
}

bool CheckoutProgressWizardPage::isComplete() const
{
    return m_state == Succeeded;
}

} // namespace Internal
} // namespace VcsBase
