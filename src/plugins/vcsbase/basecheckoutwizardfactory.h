/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASECHECKOUTWIZARDFACTORY_H
#define BASECHECKOUTWIZARDFACTORY_H

#include "vcsbase_global.h"
#include <coreplugin/iwizardfactory.h>

#include <QSharedPointer>
#include <QList>

QT_BEGIN_NAMESPACE
class QWizardPage;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal { class BaseCheckoutWizardFactoryPrivate; }

class BaseCheckoutWizard;
class Command;

class VCSBASE_EXPORT BaseCheckoutWizardFactory : public Core::IWizardFactory
{
    Q_OBJECT

public:
    BaseCheckoutWizardFactory();
    ~BaseCheckoutWizardFactory();

    void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &extraValues);

    static QString openProject(const QString &path, QString *errorMessage);

    virtual BaseCheckoutWizard *create(const QString &path, QWidget *parent = 0) const = 0;

private:
    Internal::BaseCheckoutWizardFactoryPrivate *const d;
};

} // namespace VcsBase

#endif // BASECHECKOUTWIZARDFACTORY_H
