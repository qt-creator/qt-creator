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

#ifndef CHECKOUTWIZARDDIALOG_H
#define CHECKOUTWIZARDDIALOG_H

#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <QtGui/QWizard>

namespace VCSBase {
class AbstractCheckoutJob;

namespace Internal {
class CheckoutProgressWizardPage;

/* See BaseCheckoutWizard.
 * Overwrites reject() to first kill the checkout
 * and then close. */

class CheckoutWizardDialog : public QWizard {
    Q_OBJECT
public:
    CheckoutWizardDialog(const QList<QWizardPage *> &parameterPages,
                         QWidget *parent = 0);

    void start(const QSharedPointer<AbstractCheckoutJob> &job);

signals:
    void progressPageShown();

private slots:
    void slotPageChanged(int id);
    void slotTerminated(bool success);
    virtual void reject();

private:
    CheckoutProgressWizardPage *m_progressPage;
    int m_progressPageId;
};

} // namespace Internal
} // namespace VCSBase
#endif // CHECKOUTWIZARDDIALOG_H
