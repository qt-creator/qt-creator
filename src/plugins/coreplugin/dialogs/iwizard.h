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

#ifndef IWIZARD_H
#define IWIZARD_H

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>
#include <coreplugin/featureprovider.h>

#include <QIcon>
#include <QObject>
#include <QVariantMap>

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

    class CORE_EXPORT Data
    {
    public:
        Data() : kind(IWizard::FileWizard) {}

        IWizard::WizardKind kind;
        QIcon icon;
        QString description;
        QString displayName;
        QString id;
        QString category;
        QString displayCategory;
        FeatureSet requiredFeatures;
        IWizard::WizardFlags flags;
        QString descriptionImage;
    };

    IWizard(QObject *parent = 0) : QObject(parent) {}
    ~IWizard() {}

    QString id() const { return m_data.id; }
    WizardKind kind() const { return m_data.kind; }
    QIcon icon() const { return m_data.icon; }
    QString description() const { return m_data.description; }
    QString displayName() const { return m_data.displayName; }
    QString category() const { return m_data.category; }
    QString displayCategory() const { return m_data.displayCategory; }
    QString descriptionImage() const { return m_data.descriptionImage; }
    FeatureSet requiredFeatures() const { return m_data.requiredFeatures; }
    WizardFlags flags() const { return m_data.flags; }

    void setData(const Data &data) { m_data = data; }
    void setId(const QString &id) { m_data.id = id; }
    void setWizardKind(WizardKind kind) { m_data.kind = kind; }
    void setIcon(const QIcon &icon) { m_data.icon = icon; }
    void setDescription(const QString &description) { m_data.description = description; }
    void setDisplayName(const QString &displayName) { m_data.displayName = displayName; }
    void setCategory(const QString &category) { m_data.category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_data.displayCategory = displayCategory; }
    void setDescriptionImage(const QString &descriptionImage) { m_data.descriptionImage = descriptionImage; }
    void setRequiredFeatures(const FeatureSet &featureSet) { m_data.requiredFeatures = featureSet; }
    void addRequiredFeature(const Feature &feature) { m_data.requiredFeatures |= feature; }
    void setFlags(WizardFlags flags) { m_data.flags = flags; }

    virtual void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &variables) = 0;

    bool isAvailable(const QString &platformName) const;
    QStringList supportedPlatforms() const;

    // Utility to find all registered wizards
    static QList<IWizard*> allWizards();
    // Utility to find all registered wizards of a certain kind
    static QList<IWizard*> wizardsOfKind(WizardKind kind);
    static QStringList allAvailablePlatforms();
    static QString displayNameForPlatform(const QString &string);

private:
    Data m_data;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardKinds)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizard::WizardFlags)

#endif // IWIZARD_H
