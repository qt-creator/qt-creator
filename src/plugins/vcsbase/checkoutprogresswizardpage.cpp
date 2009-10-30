/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "checkoutprogresswizardpage.h"
#include "checkoutjobs.h"
#include "ui_checkoutprogresswizardpage.h"

#include <utils/qtcassert.h>

#include <QtGui/QApplication>
#include <QtGui/QCursor>

namespace VCSBase {
namespace Internal {

CheckoutProgressWizardPage::CheckoutProgressWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CheckoutProgressWizardPage),
    m_state(Idle)
{
    ui->setupUi(this);
}

CheckoutProgressWizardPage::~CheckoutProgressWizardPage()
{
    if (m_state == Running) // Paranoia!
        QApplication::restoreOverrideCursor();
    delete ui;
}

void CheckoutProgressWizardPage::start(const QSharedPointer<AbstractCheckoutJob> &job)
{
    QTC_ASSERT(m_state != Running, return)
    m_job = job;
    connect(job.data(), SIGNAL(output(QString)), ui->logPlainTextEdit, SLOT(appendPlainText(QString)));
    connect(job.data(), SIGNAL(failed(QString)), this, SLOT(slotFailed(QString)));
    connect(job.data(), SIGNAL(succeeded()), this, SLOT(slotSucceeded()));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->logPlainTextEdit->clear();
    setSubTitle(tr("Checkout started..."));    
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
        setSubTitle(tr("Failed."));
        emit terminated(false);
    }
}

void CheckoutProgressWizardPage::slotSucceeded()
{
    if (m_state == Running) {
        m_state = Succeeded;        
        QApplication::restoreOverrideCursor();
        setSubTitle(tr("Succeeded."));
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
} // namespace VCSBase
