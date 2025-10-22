// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QJsonObject>
#include <QLabel>
#include <QPointer>
#include <QUuid>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class FilePath;
class PathChooser;
class FancyLineEdit;
} // namespace Utils

namespace Core { class IDocument; }

namespace ProjectExplorer {
class BuildConfiguration;
class Project;
}

namespace TextEditor { class BaseTextEditor; }

namespace LanguageClient {

class Client;
class BaseClientInterface;

class LANGUAGECLIENT_EXPORT LanguageFilter
{
public:
    QStringList mimeTypes;
    QStringList filePattern;
    bool isSupported(const Utils::FilePath &filePath, const QString &mimeType) const;
    bool isSupported(const Core::IDocument *document) const;
    bool operator==(const LanguageFilter &other) const;
    bool operator!=(const LanguageFilter &other) const;
};

class LANGUAGECLIENT_EXPORT BaseSettings : public Utils::AspectContainer
{
public:
    BaseSettings();

    virtual ~BaseSettings() = default;

    enum StartBehavior {
        AlwaysOn = 0,
        RequiresFile,
        RequiresProject,
        LastSentinel
    };

    Utils::StringAspect name{this};

    QString m_id = QUuid::createUuid().toString();
    Utils::Id m_settingsTypeId;
    bool m_enabled = true;
    StartBehavior m_startBehavior = RequiresFile;
    LanguageFilter m_languageFilter;
    Utils::StringAspect initializationOptions{this};
    Utils::StringAspect configuration{this};
    Utils::BoolAspect showInSettings{this};
    // controls whether the resulting client can be used for completions/highlight/outline etc.
    Utils::BoolAspect activatable{this};

    QJsonObject initializationOptionsAsJson() const;
    QJsonValue configurationAsJson() const;

    virtual bool applyFromSettingsWidget(QWidget *widget);
    virtual QWidget *createSettingsWidget(QWidget *parent = nullptr) const;
    virtual BaseSettings *copy() const;
    virtual BaseSettings *create() const = 0;
    virtual bool isValid() const;
    virtual bool isValidOnBuildConfiguration(ProjectExplorer::BuildConfiguration *bc) const;
    Client *createClient() const;
    Client *createClient(ProjectExplorer::BuildConfiguration *bc) const;
    bool isEnabledOnProject(ProjectExplorer::Project *project) const;

    virtual void toMap(Utils::Store &map) const;
    virtual void fromMap(const Utils::Store &map);

protected:
    virtual BaseClientInterface *createInterface(ProjectExplorer::BuildConfiguration *) const = 0;
    virtual Client *createClient(BaseClientInterface *interface) const;
};

class LANGUAGECLIENT_EXPORT StdIOSettings : public BaseSettings
{
public:
    StdIOSettings();
    ~StdIOSettings() override;

    QWidget *createSettingsWidget(QWidget *parent = nullptr) const override;
    BaseSettings *create() const override { return new StdIOSettings; }
    bool isValid() const override;

    Utils::CommandLine command() const;

    Utils::StringAspect arguments{this};
    Utils::FilePathAspect executable{this};

protected:
    BaseClientInterface *createInterface(ProjectExplorer::BuildConfiguration *bc) const override;
};

struct ClientType {
    Utils::Id id;
    QString name;
    using SettingsGenerator = std::function<BaseSettings*()>;
    SettingsGenerator generator = nullptr;
    bool userAddable = true;
};

class LANGUAGECLIENT_EXPORT LanguageClientSettings
{
public:
    static void init();
    static bool initialized();

    static QList<BaseSettings *> fromSettings(Utils::QtcSettings *settings);
    static QList<BaseSettings *> pageSettings();
    static QList<BaseSettings *> changedSettings();

    static QList<Utils::Store> storesBySettingsType(Utils::Id settingsTypeId);

    /**
     * must be called before the delayed initialize phase
     * otherwise the settings are not loaded correctly
     */
    static void registerClientType(const ClientType &type);
    static void addSettings(BaseSettings *settings);
    static void enableSettings(const QString &id, bool enable = true);
    static void toSettings(Utils::QtcSettings *settings, const QList<BaseSettings *> &languageClientSettings);

    static bool outlineComboBoxIsSorted();
    static void setOutlineComboBoxSorted(bool sorted);
};

class LANGUAGECLIENT_EXPORT BaseSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseSettingsWidget(
        const BaseSettings *settings,
        QWidget *parent = nullptr,
        Layouting::LayoutModifier additionalItems = {});

    ~BaseSettingsWidget() override = default;

    LanguageFilter filter() const;
    BaseSettings::StartBehavior startupBehavior() const;
    bool alwaysOn() const;
    bool requiresProject() const;
    QString initializationOptions() const;

private:
    void showAddMimeTypeDialog();

    QLabel *m_mimeTypes = nullptr;
    QLineEdit *m_filePattern = nullptr;
    QComboBox *m_startupBehavior = nullptr;
    Utils::FancyLineEdit *m_initializationOptions = nullptr;

    static constexpr char filterSeparator = ';';
};

class LANGUAGECLIENT_EXPORT StdIOSettingsWidget : public BaseSettingsWidget
{
    Q_OBJECT
public:
    explicit StdIOSettingsWidget(const StdIOSettings* settings, QWidget *parent = nullptr);
    ~StdIOSettingsWidget() override = default;
};

class ProjectSettings
{
public:
    explicit ProjectSettings(ProjectExplorer::Project *project);

    QJsonValue workspaceConfiguration() const;

    QByteArray json() const;
    void setJson(const QByteArray &json);

    void enableSetting(const QString &id);
    void disableSetting(const QString &id);
    void clearOverride(const QString &id);

    QStringList enabledSettings();
    QStringList disabledSettings();

private:
    ProjectExplorer::Project *m_project = nullptr;
    QByteArray m_json;
    QStringList m_enabledSettings;
    QStringList m_disabledSettings;
};

LANGUAGECLIENT_EXPORT TextEditor::BaseTextEditor *createJsonEditor(QObject *parent = nullptr);

void setupLanguageClientProjectPanel();

} // namespace LanguageClient
