/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef IWIZARD_H
#define IWIZARD_H

#include <coreplugin/core_global.h>
#include <coreplugin/featureprovider.h>

#include <QObject>
#include <QVariantMap>

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
    enum WizardFlag {
        PlatformIndependent = 0x01,
        ForceCapitalLetterForFileName = 0x02
    };
    Q_DECLARE_FLAGS(WizardFlags, WizardFlag)

    IWizard(QObject *parent = 0) : QObject(parent) {}
    virtual ~IWizard() {}

    virtual WizardKind kind() const = 0;
    virtual QIcon icon() const = 0;
    virtual QString description() const = 0;
    virtual QString displayName() const = 0;
    virtual QString id() const = 0;

    virtual QString category() const = 0;
    virtual QString displayCategory() const = 0;

    virtual QString descriptionImage() const = 0;

    virtual FeatureSet requiredFeatures() const = 0;
    virtual WizardFlags flags() const = 0;

    virtual void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &variables) = 0;

    bool isAvailable(const QString &platformName) const;
    QStringList supportedPlatforms() const;

    // Utility to find all registered wizards
    static QList<IWizard*> allWizards();
    // Utility to find all registered wizards of a certain kind
    static QList<IWizard*> wizardsOfKind(WizardKind kind);
    static QStringList allAvailablePlatforms();
    static QString displayNameForPlatform(const QString &string);
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardKinds)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardFlags)

#endif // IWIZARD_H
