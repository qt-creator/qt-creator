/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CHECKOUTPROGRESSWIZARDPAGE_H
#define CHECKOUTPROGRESSWIZARDPAGE_H

#include <QtCore/QSharedPointer>
#include <QtGui/QWizardPage>

namespace VCSBase {
class AbstractCheckoutJob;

namespace Internal {

namespace Ui {
    class CheckoutProgressWizardPage;
}

/* Page showing the progress of an initial project
 * checkout. Turns complete when the job succeeds. */

class CheckoutProgressWizardPage : public QWizardPage {
    Q_OBJECT
    Q_DISABLE_COPY(CheckoutProgressWizardPage)

public:
    enum State { Idle, Running, Failed, Succeeded };

    explicit CheckoutProgressWizardPage(QWidget *parent = 0);
    ~CheckoutProgressWizardPage();

    void start(const QSharedPointer<AbstractCheckoutJob> &job);

    virtual bool isComplete() const;
    bool isRunning() const{ return m_state == Running; }

    void terminate();

signals:
    void terminated(bool success);

private slots:
    void slotFailed(const QString &);
    void slotSucceeded();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::CheckoutProgressWizardPage *ui;
    QSharedPointer<AbstractCheckoutJob> m_job;

    State m_state;
};

} // namespace Internal
} // namespace VCSBase
#endif // CHECKOUTPROGRESSWIZARDPAGE_H
