/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef IWIZARD_H
#define IWIZARD_H

#include <coreplugin/core_global.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IWizard
    : public QObject
{
    Q_OBJECT
public:
    enum WizardKind {
        FileWizard = 0x01,
        ClassWizard = 0x02,
        ProjectWizard = 0x04
    };
    Q_DECLARE_FLAGS(WizardKinds, WizardKind)

    IWizard(QObject *parent = 0) : QObject(parent) {}
    virtual ~IWizard() {}

    virtual WizardKind kind() const = 0;
    virtual QIcon icon() const = 0;
    virtual QString description() const = 0;
    virtual QString displayName() const = 0;
    virtual QString id() const = 0;

    virtual QString category() const = 0;
    virtual QString displayCategory() const = 0;

    virtual void runWizard(const QString &path, QWidget *parent) = 0;

    // Utility to find all registered wizards
    static QList<IWizard*> allWizards();
    // Utility to find all registered wizards of a certain kind
    static QList<IWizard*> wizardsOfKind(WizardKind kind);
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardKinds)

#endif // IWIZARD_H
