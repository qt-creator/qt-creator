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

#ifndef BASECHECKOUTWIZARD_H
#define BASECHECKOUTWIZARD_H

#include "vcsbase_global.h"
#include <coreplugin/dialogs/iwizard.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QWizardPage;
QT_END_NAMESPACE

namespace VCSBase {

class AbstractCheckoutJob;
struct BaseCheckoutWizardPrivate;

/* A Core::IWizard implementing a wizard for initially checking
 * out a project using a version control system.
 * Implements all of Core::IWizard with the exception of
 * name()/description() and icon().
 * Pops up a QWizard consisting of a Parameter Page which is created
 * by a virtual factory function and a progress
 * page containing a log text. The factory function createJob()
 * creates a job with the output connected to the log window,
 * returning the path to the checkout.
 * On success, the wizard tries to locate a project file
 * and open it.
 * BaseCheckoutWizardPage is provided as a convenience base class
 * for parameter wizard pages. */

class VCSBASE_EXPORT BaseCheckoutWizard : public Core::IWizard
{
    Q_OBJECT

public:
    explicit BaseCheckoutWizard(QObject *parent = 0);
    virtual ~BaseCheckoutWizard();

    virtual WizardKind kind() const;

    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QString id() const;

    virtual void runWizard(const QString &path, QWidget *parent);

    static QString openProject(const QString &path, QString *errorMessage);

protected:
    virtual QList<QWizardPage *> createParameterPages(const QString &path) = 0;
    virtual QSharedPointer<AbstractCheckoutJob> createJob(const QList<QWizardPage *> &parameterPages,
                                                          QString *checkoutPath) = 0;

public slots:
    void setId(const QString &id);

private slots:
    void slotProgressPageShown();

private:
    BaseCheckoutWizardPrivate *d;
};

} // namespace VCSBase

#endif // BASECHECKOUTWIZARD_H
