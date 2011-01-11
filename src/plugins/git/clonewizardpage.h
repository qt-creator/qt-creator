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

#ifndef CLONEWIZARDPAGE_H
#define CLONEWIZARDPAGE_H

#include <vcsbase/basecheckoutwizardpage.h>

#include <QtCore/QSharedPointer>

namespace VCSBase {
    class AbstractCheckoutJob;
}

namespace Git {

struct CloneWizardPagePrivate;

// Used by gitorious as well.
class CloneWizardPage : public VCSBase::BaseCheckoutWizardPage
{
    Q_OBJECT
    Q_PROPERTY(bool deleteMasterBranch READ deleteMasterBranch WRITE setDeleteMasterBranch)
public:
    explicit CloneWizardPage(QWidget *parent = 0);
    virtual ~CloneWizardPage();

    QSharedPointer<VCSBase::AbstractCheckoutJob> createCheckoutJob(QString *checkoutPath) const;

protected:
    virtual QString directoryFromRepository(const QString &r) const;
    virtual QStringList branches(const QString &repository, int *current);

    bool deleteMasterBranch() const;
    void setDeleteMasterBranch(bool v);

private:
    CloneWizardPagePrivate *d;
};

} // namespace Git
#endif // CLONEWIZARDPAGE_H
