/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
