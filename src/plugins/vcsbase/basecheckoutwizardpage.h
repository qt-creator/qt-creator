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

#ifndef VCSBASE_CHECKOUTWIZARDPAGE_H
#define VCSBASE_CHECKOUTWIZARDPAGE_H

#include "vcsbase_global.h"

#include <QtGui/QWizardPage>

namespace VCSBase {

namespace Ui {
    class BaseCheckoutWizardPage;
}

struct BaseCheckoutWizardPagePrivate;

/* Base class for a parameter page of a checkout wizard.
 * Let's the user specify the repository, a checkout directory and
 * the path. Contains a virtual to derive the checkout directory
 * from the repository as it is entered. */

class VCSBASE_EXPORT BaseCheckoutWizardPage : public QWizardPage {
    Q_OBJECT
public:
    BaseCheckoutWizardPage(QWidget *parent = 0);
    ~BaseCheckoutWizardPage();

    QString path() const;
    void setPath(const QString &);

    QString directory() const;
    void setDirectory(const QString &d);

    QString repository() const;
    void setRepository(const QString &r);

    bool isRepositoryReadOnly() const;
    void setRepositoryReadOnly(bool v);

    virtual bool isComplete() const;

protected:
    void changeEvent(QEvent *e);

    void setRepositoryLabel(const QString &l);
    void setDirectoryVisible(bool v);

    /* Determine a checkout directory name from
     * repository URL, that is, "protocol:/project" -> "project". */
    virtual QString directoryFromRepository(const QString &r) const;

private slots:
    void slotRepositoryChanged(const QString &url);
    void slotDirectoryEdited();
    void slotChanged();

private:
    BaseCheckoutWizardPagePrivate *d;
};

} // namespace VCSBase

#endif // VCSBASE_CHECKOUTWIZARDPAGE_H
