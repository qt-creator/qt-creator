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

#include "checkoutprogresswizardpage.h"
#include "checkoutjobs.h"
#include "ui_checkoutprogresswizardpage.h"

#include <utils/qtcassert.h>

#include <QApplication>
#include <QCursor>

/*!
    \class VcsBase::CheckoutProgressWizardPage

    \brief Page showing the progress of an initial project checkout. Turns complete when the job
           succeeds.

    \sa VcsBase::BaseCheckoutWizard
*/

namespace VcsBase {
namespace Internal {

CheckoutProgressWizardPage::CheckoutProgressWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CheckoutProgressWizardPage),
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

void CheckoutProgressWizardPage::start(const QSharedPointer<AbstractCheckoutJob> &job)
{
    if (job.isNull()) {
        ui->logPlainTextEdit->setPlainText(tr("No job running, please abort."));
        return;
    }

    QTC_ASSERT(m_state != Running, return);
    m_job = job;
    connect(job.data(), SIGNAL(output(QString)), ui->logPlainTextEdit, SLOT(appendPlainText(QString)));
    connect(job.data(), SIGNAL(failed(QString)), this, SLOT(slotFailed(QString)));
    connect(job.data(), SIGNAL(succeeded()), this, SLOT(slotSucceeded()));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->logPlainTextEdit->clear();
    ui->statusLabel->setText(tr("Checkout started..."));
    ui->statusLabel->setPalette(QPalette());
    m_state = Running;
    // Note: Process jobs can emit failed() right from
    // the start() method on Windows.
    job->start();
}

void CheckoutProgressWizardPage::slotFailed(const QString &why)
{
    ui->logPlainTextEdit->appendPlainText(why);
    if (m_state == Running) {
        m_state = Failed;
        QApplication::restoreOverrideCursor();
        ui->statusLabel->setText(tr("Failed."));
        QPalette palette = ui->statusLabel->palette();
        palette.setColor(QPalette::Active, QPalette::Text, Qt::red);
        ui->statusLabel->setPalette(palette);
        emit terminated(false);
    }
}

void CheckoutProgressWizardPage::slotSucceeded()
{
    if (m_state == Running) {
        m_state = Succeeded;
        QApplication::restoreOverrideCursor();
        ui->statusLabel->setText(tr("Succeeded."));
        QPalette palette = ui->statusLabel->palette();
        palette.setColor(QPalette::Active, QPalette::Text, Qt::green);
        ui->statusLabel->setPalette(palette);
        emit completeChanged();
        emit terminated(true);
    }
}

void CheckoutProgressWizardPage::terminate()
{
    if (!m_job.isNull())
        m_job->cancel();
}

bool CheckoutProgressWizardPage::isComplete() const
{
    return m_state == Succeeded;
}

void CheckoutProgressWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace VcsBase
