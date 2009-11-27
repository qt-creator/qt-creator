/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef CLONEWIZARD_H
#define CLONEWIZARD_H

#include <vcsbase/basecheckoutwizard.h>

#include <QtGui/QIcon>

namespace Mercurial {
namespace Internal {

class CloneWizard : public VCSBase::BaseCheckoutWizard
{
    Q_OBJECT
public:
    CloneWizard(QObject *parent = 0);

    QIcon icon() const;
    QString description() const;
    QString name() const;

protected:
    QList<QWizardPage *> createParameterPages(const QString &path);
    QSharedPointer<VCSBase::AbstractCheckoutJob> createJob(const QList<QWizardPage *> &parameterPages,
                                                           QString *checkoutPath);

private:
    const QIcon m_icon;
};

} //namespace Internal
} //namespace Mercurial

#endif // CLONEWIZARD_H
