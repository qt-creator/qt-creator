/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef BASECHECKOUTWIZARD_H
#define BASECHECKOUTWIZARD_H

#include "vcsbase_global.h"
#include <coreplugin/dialogs/iwizard.h>

#include <QSharedPointer>
#include <QList>

QT_BEGIN_NAMESPACE
class QWizardPage;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal {
class BaseCheckoutWizardPrivate;
}

class AbstractCheckoutJob;

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

    virtual QString descriptionImage() const;

    virtual void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &extraValues);

    virtual Core::FeatureSet requiredFeatures() const;

    virtual WizardFlags flags() const;

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
    Internal::BaseCheckoutWizardPrivate *const d;
};

} // namespace VcsBase

#endif // BASECHECKOUTWIZARD_H
