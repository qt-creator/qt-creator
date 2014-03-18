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
#include "ui_checkoutprogresswizardpage.h"
#include "command.h"
#include "vcsbaseplugin.h"

#include <utils/qtcassert.h>

#include <QApplication>

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
    ui(new Ui::CheckoutProgressWizardPage),
    m_startedStatus(tr("Checkout started...")),
    m_overwriteOutput(false),
    m_state(Idle)
{
    ui->setupUi(this);
    setTitle(tr("Checkout"));
}

CheckoutProgressWizardPage::~CheckoutProgressWizardPage()
{
    if (m_state == Running) // Paranoia!
        QApplication::restoreOverrideCursor();
    delete ui;
}

void CheckoutProgressWizardPage::setStartedStatus(const QString &startedStatus)
{
    m_startedStatus = startedStatus;
}

void CheckoutProgressWizardPage::start(Command *command)
{
    if (!command) {
        ui->logPlainTextEdit->setPlainText(tr("No job running, please abort."));
        return;
    }

    QTC_ASSERT(m_state != Running, return);
    m_command = command;
    command->setProgressiveOutput(true);
    connect(command, SIGNAL(output(QString)), this, SLOT(slotOutput(QString)));
    connect(command, SIGNAL(finished(bool,int,QVariant)), this, SLOT(slotFinished(bool,int,QVariant)));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->logPlainTextEdit->clear();
    m_overwriteOutput = false;
    ui->statusLabel->setText(m_startedStatus);
    ui->statusLabel->setPalette(QPalette());
    m_state = Running;
    command->execute();
}

void CheckoutProgressWizardPage::slotFinished(bool ok, int exitCode, const QVariant &)
{
    if (ok && exitCode == 0) {
        if (m_state == Running) {
            m_state = Succeeded;
            QApplication::restoreOverrideCursor();
            ui->statusLabel->setText(tr("Succeeded."));
            QPalette palette;
            palette.setColor(QPalette::Active, QPalette::Text, Qt::green);
            ui->statusLabel->setPalette(palette);
            emit completeChanged();
            emit terminated(true);
        }
    } else {
        ui->logPlainTextEdit->appendPlainText(m_error);
        if (m_state == Running) {
            m_state = Failed;
            QApplication::restoreOverrideCursor();
            ui->statusLabel->setText(tr("Failed."));
            QPalette palette;
            palette.setColor(QPalette::Active, QPalette::Text, Qt::red);
            ui->statusLabel->setPalette(palette);
            emit terminated(false);
        }
    }
}

void CheckoutProgressWizardPage::slotOutput(const QString &text)
{
    int startPos = 0;
    int crPos = -1;
    const QString ansiEraseToEol = QLatin1String("\x1b[K");
    while ((crPos = text.indexOf(QLatin1Char('\r'), startPos)) >= 0)  {
        QString part = text.mid(startPos, crPos - startPos);
        // Discard ANSI erase-to-eol
        if (part.endsWith(ansiEraseToEol))
            part.chop(ansiEraseToEol.length());
        outputText(part);
        startPos = crPos + 1;
        m_overwriteOutput = true;
    }
    if (startPos < text.count())
        outputText(text.mid(startPos));
}

void CheckoutProgressWizardPage::slotError(const QString &text)
{
    m_error.append(text);
}

void CheckoutProgressWizardPage::outputText(const QString &text)
{
    if (m_overwriteOutput) {
        QTextCursor cursor = ui->logPlainTextEdit->textCursor();
        cursor.clearSelection();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        ui->logPlainTextEdit->setTextCursor(cursor);
        m_overwriteOutput = false;
    }
    ui->logPlainTextEdit->insertPlainText(text);
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
