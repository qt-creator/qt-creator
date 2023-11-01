// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "featureprovider.h"

#include <QIcon>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Utils {
class FilePath;
class Wizard;
} // Utils

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

    Utils::Id id() const { return m_id; }
    WizardKind kind() const { return m_supportedProjectTypes.isEmpty() ? FileWizard : ProjectWizard; }
    QIcon icon() const { return m_icon; }
    QString fontIconName() const { return m_fontIconName; }
    QString description() const { return m_description; }
    QString displayName() const { return m_displayName; }
    QString category() const { return m_category; }
    QString displayCategory() const { return m_displayCategory; }
    QString descriptionImage() const { return m_descriptionImage; }
    QSet<Utils::Id> requiredFeatures() const { return m_requiredFeatures; }
    WizardFlags flags() const { return m_flags; }
    QUrl detailsPageQmlPath() const { return m_detailsPageQmlPath; }

    QSet<Utils::Id> supportedProjectTypes() const { return m_supportedProjectTypes; }

    void setId(const Utils::Id id) { m_id = id; }
    void setSupportedProjectTypes(const QSet<Utils::Id> &projectTypes) { m_supportedProjectTypes = projectTypes; }
    void setIcon(const QIcon &icon, const QString &iconText = {});
    void setFontIconName(const QString &code) { m_fontIconName = code; }
    void setDescription(const QString &description) { m_description = description; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setCategory(const QString &category) { m_category = category; }
    void setDisplayCategory(const QString &displayCategory) { m_displayCategory = displayCategory; }
    void setDescriptionImage(const QString &descriptionImage) { m_descriptionImage = descriptionImage; }
    void setRequiredFeatures(const QSet<Utils::Id> &featureSet) { m_requiredFeatures = featureSet; }
    void addRequiredFeature(const Utils::Id &feature) { m_requiredFeatures |= feature; }
    void setFlags(WizardFlags flags) { m_flags = flags; }
    void setDetailsPageQmlPath(const QString &filePath);

    Utils::FilePath runPath(const Utils::FilePath &defaultPath) const;

    // Does bookkeeping and the calls runWizardImpl. Please implement that.
    Utils::Wizard *runWizard(const Utils::FilePath &path, QWidget *parent, Utils::Id platform,
                             const QVariantMap &variables, bool showWizard = true);

    virtual bool isAvailable(Utils::Id platformId) const;
    QSet<Utils::Id> supportedPlatforms() const;

    using FactoryCreator = std::function<IWizardFactory *()>;
    static void registerFactoryCreator(const FactoryCreator &creator);

    // Utility to find all registered wizards
    static QList<IWizardFactory*> allWizardFactories();
    static QSet<Utils::Id> allAvailablePlatforms();
    static QString displayNameForPlatform(Utils::Id i);

    static void registerFeatureProvider(IFeatureProvider *provider);

    static bool isWizardRunning();
    static QWidget *currentWizard();

    static void requestNewItemDialog(const QString &title,
                                     const QList<IWizardFactory *> &factories,
                                     const Utils::FilePath &defaultLocation,
                                     const QVariantMap &extraVariables);

    static QIcon themedIcon(const Utils::FilePath &iconMaskPath);

protected:
    static QSet<Utils::Id> pluginFeatures();
    static QSet<Utils::Id> availableFeatures(Utils::Id platformId);

    virtual Utils::Wizard *runWizardImpl(const Utils::FilePath &path,
                                         QWidget *parent,
                                         Utils::Id platform,
                                         const QVariantMap &variables,
                                         bool showWizard = true) = 0;

private:
    static void initialize();
    static void destroyFeatureProvider();

    static void clearWizardFactories();

    QAction *m_action = nullptr;
    QIcon m_icon;
    QString m_fontIconName;
    QString m_description;
    QString m_displayName;
    QString m_category;
    QString m_displayCategory;
    QString m_descriptionImage;
    QUrl m_detailsPageQmlPath;
    QSet<Utils::Id> m_requiredFeatures;
    QSet<Utils::Id> m_supportedProjectTypes;
    WizardFlags m_flags;
    Utils::Id m_id;

    friend class Internal::CorePlugin;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::IWizardFactory::WizardFlags)
