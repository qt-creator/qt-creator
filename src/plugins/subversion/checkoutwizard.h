/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CHECKOUTWIZARD_H
#define CHECKOUTWIZARD_H

#include <vcsbase/basecheckoutwizard.h>

namespace Subversion {
namespace Internal {

class CheckoutWizard : public VCSBase::BaseCheckoutWizard
{
    Q_OBJECT
public:
    explicit CheckoutWizard(QObject *parent = 0);

    // IWizard
    virtual QIcon icon() const;
    virtual QString description() const;
    virtual QString displayName() const;

protected:
    // BaseCheckoutWizard
    virtual QList<QWizardPage*> createParameterPages(const QString &path);
    virtual QSharedPointer<VCSBase::AbstractCheckoutJob> createJob(const QList<QWizardPage*> &parameterPage,
                                                                   QString *checkoutPath);
};

} // namespace Internal
} // namespace Subversion

#endif // CHECKOUTWIZARD_H
