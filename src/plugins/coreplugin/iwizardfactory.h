/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IWIZARDFACTORY_H
#define IWIZARDFACTORY_H

#include <coreplugin/core_global.h>
#include <coreplugin/featureprovider.h>

#include <QIcon>
#include <QObject>
#include <QString>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Utils { class Wizard; }

namespace Core {

namespace Internal { class CorePlugin; }

class CORE_EXPORT IWizardFactory
    : public QObject
{
    Q_OBJECT
public:
    enum WizardKind {
        FileWizard = 0x01,
        ProjectWizard = 0x02
    };
    enum WizardFlag {
        PlatformIndependent = 0x01,
        ForceCapitalLetterForFileName = 0x02
    };
    Q_DECLARE_FLAGS(WizardFlags, WizardFlag)

    IWizardFactory() : m_kind(FileWizard) { }

    Id id() const { return m_id; }
    WizardKind kind() const { return m_kind; }
    QIcon icon() const { return m_icon; }
    QString description() const { return m_description; }
    QString displayName() const { return m_displayName; }
    QString category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QString descriptionImage() const { return m_descriptionImage; }
    FeatureSet requiredFeatures() const { return m_requiredFeatures; }
    WizardFlags flags() const { return m_flags; }

    void setId(const Id id) { m_id = id; }
    void setWizardKind(WizardKind kind) { m_kind = kind; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setDescription(const QString &description) { m_description = description; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setCategory(const QString &category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setDescriptionImage(const QString &descriptionImage) { m_descriptionImage = descriptionImage; }
    void setRequiredFeatures(const FeatureSet &featureSet) { m_requiredFeatures = featureSet; }
    void addRequiredFeature(const Feature &feature) { m_requiredFeatures |= feature; }
    void setFlags(WizardFlags flags) { m_flags = flags; }

    QString runPath(const QString &defaultPath);

    // Does bookkeeping and the calls runWizardImpl. Please implement that.
    virtual Utils::Wizard *runWizard(const QString &path, QWidget *parent, const QString &platform,
                                     const QVariantMap &variables);

    virtual bool isAvailable(const QString &platformName) const;
    QStringList supportedPlatforms() const;

    typedef std::function<QList<IWizardFactory *>()> FactoryCreator;
    static void registerFactoryCreator(const FactoryCreator &creator);

    // Utility to find all registered wizards
    static QList<IWizardFactory*> allWizardFactories();
    // Utility to find all registered wizards of a certain kind
    static QList<IWizardFactory*> wizardFactoriesOfKind(WizardKind kind);
    static QStringList allAvailablePlatforms();
    static QString displayNameForPlatform(const QString &string);

    static void registerFeatureProvider(IFeatureProvider *provider);

    static bool isWizardRunning();

protected:
    FeatureSet pluginFeatures() const;
    FeatureSet availableFeatures(const QString &platformName) const;

    virtual Utils::Wizard *runWizardImpl(const QString &path, QWidget *parent, const QString &platform,
                                         const QVariantMap &variables) = 0;

private:
    static void initialize();
    static void destroyFeatureProvider();

    static void clearWizardFactories();

    QAction *m_action = 0;
    IWizardFactory::WizardKind m_kind;
    QIcon m_icon;
    QString m_description;
    QString m_displayName;
    Id m_id;
    QString m_category;
    QString m_displayCategory;
    FeatureSet m_requiredFeatures;
    WizardFlags m_flags;
    QString m_descriptionImage;

    friend class Internal::CorePlugin;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizardFactory::WizardFlags)

#endif // IWIZARDFACTORY_H
