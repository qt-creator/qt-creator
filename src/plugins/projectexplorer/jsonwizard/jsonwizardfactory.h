// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include "jsonwizard.h"

#include <coreplugin/iwizardfactory.h>

#include <utils/filepath.h>

#include <QVariant>

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class ProjectExplorerPluginPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT JsonWizardFactory : public Core::IWizardFactory
{
    Q_OBJECT

public:
    // Add search paths for wizard.json files. All subdirs are going to be checked.
    static void addWizardPath(const Utils::FilePath &path);
    static void clearWizardPaths();

    // actual interface of the wizard factory:
    class Generator {
    public:
        bool isValid() const { return typeId.isValid(); }

        Utils::Id typeId;
        QVariant data;
    };

    class Page {
    public:
        bool isValid() const { return typeId.isValid(); }
        QString title;
        QString subTitle;
        QString shortTitle;
        int index = -1; // page index in the wizard
        Utils::Id typeId;
        QVariant enabled;
        QVariant data;
    };

    static QList<QVariant> objectOrList(const QVariant &data, QString *errorMessage);

    static QString localizedString(const QVariant &value);

    bool isAvailable(Utils::Id platformId) const override;

    virtual std::pair<int, QStringList> screenSizeInfoFromPage(const QString &pageType) const;

private:
    Utils::Wizard *runWizardImpl(const Utils::FilePath &path, QWidget *parent, Utils::Id platform,
                                 const QVariantMap &variables, bool showWizard = true) override;

    // Create all wizards. As other plugins might register factories for derived
    // classes. Called when the new file dialog is shown for the first time.
    static void createWizardFactories();
    static JsonWizardFactory *createWizardFactory(const QVariantMap &data,
                                                  const Utils::FilePath &baseDir,
                                                  QString *errorMessage);
    static Utils::FilePaths &searchPaths();

    static void setVerbose(int level);
    static int verbose();

    bool initialize(const QVariantMap &data, const Utils::FilePath &baseDir, QString *errorMessage);

    JsonWizardFactory::Page parsePage(const QVariant &value, QString *errorMessage);
    QVariantMap loadDefaultValues(const QString &fileName);
    QVariant getDataValue(const QLatin1String &key, const QVariantMap &valueSet,
                          const QVariantMap &defaultValueSet, const QVariant &notExistValue={});
    QVariant mergeDataValueMaps(const QVariant &valueMap, const QVariant &defaultValueMap);

    QVariant m_enabledExpression;
    Utils::FilePath m_wizardDir;
    QList<Generator> m_generators;

    QList<Page> m_pages;
    QList<JsonWizard::OptionDefinition> m_options;

    QSet<Utils::Id> m_preferredFeatures;

    static int m_verbose;

    friend class ProjectExplorerPlugin;
    friend class ProjectExplorerPluginPrivate;
};

namespace Internal {

class JsonWizardFactoryJsExtension : public QObject
{
    Q_OBJECT
public:
    JsonWizardFactoryJsExtension(Utils::Id platformId,
                                 const QSet<Utils::Id> &availableFeatures,
                                 const QSet<Utils::Id> &pluginFeatures);

    Q_INVOKABLE QVariant value(const QString &name) const;

private:
    Utils::Id m_platformId;
    QSet<Utils::Id> m_availableFeatures;
    QSet<Utils::Id> m_pluginFeatures;
};

} // namespace Internal
} // namespace ProjectExplorer
