// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllsclientsettings.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"
#include "qmllsclient.h"

#include <utils/mimeconstants.h>

#include <coreplugin/messagemanager.h>

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <QtWidgets/qcheckbox.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

using namespace LanguageClient;
using namespace QtSupport;
using namespace Utils;
using namespace ProjectExplorer;

namespace QmlJSEditor {

constexpr char useLatestQmllsKey[] = "useLatestQmlls";
constexpr char disableBuiltinCodemodelKey[] = "disableBuiltinCodemodel";
constexpr char generateQmllsIniFilesKey[] = "generateQmllsIniFiles";
constexpr char ignoreMinimumQmllsVersionKey[] = "ignoreMinimumQmllsVersion";
constexpr char useQmllsSemanticHighlightingKey[] = "enableQmllsSemanticHighlighting";

QmllsClientSettings *qmllsSettings()
{
    BaseSettings *qmllsSettings
        = Utils::findOrDefault(LanguageClientManager::currentSettings(), [](BaseSettings *setting) {
              return setting->m_settingsTypeId == Constants::QMLLS_CLIENT_SETTINGS_ID;
          });
    return static_cast<QmllsClientSettings *>(qmllsSettings);
}

QmllsClientSettings::QmllsClientSettings()
{
    m_name = "QML Language Server";

    using namespace Utils::Constants;
    m_languageFilter.mimeTypes
        = {QML_MIMETYPE,
           QMLUI_MIMETYPE,
           QBS_MIMETYPE,
           QMLPROJECT_MIMETYPE,
           QMLTYPES_MIMETYPE,
           JS_MIMETYPE,
           JSON_MIMETYPE};

    m_settingsTypeId = Constants::QMLLS_CLIENT_SETTINGS_ID;
    m_startBehavior = RequiresProject;
}

static QtVersion *qtVersionFromProject(const Project *project)
{
    if (!project)
        return {};
    auto *target = project->activeTarget();
    if (!target)
        return {};
    auto *kit = target->kit();
    if (!kit)
        return {};
    auto qtVersion = QtKitAspect::qtVersion(kit);
    return qtVersion;
}

static std::pair<FilePath, QVersionNumber> evaluateLatestQmlls()
{
    // find latest qmlls, i.e. vals
    if (!QtVersionManager::isLoaded())
        return {};
    const QtVersions versions = QtVersionManager::versions();
    FilePath latestQmlls;
    QVersionNumber latestVersion;
    FilePath latestQmakeFilePath;
    int latestUniqueId = std::numeric_limits<int>::min();
    for (QtVersion *v : versions) {
        // check if we find qmlls
        QVersionNumber vNow = v->qtVersion();
        FilePath qmllsNow = QmlJS::ModelManagerInterface::qmllsForBinPath(v->hostBinPath(), vNow);
        if (!qmllsNow.isExecutableFile())
            continue;
        if (latestVersion > vNow)
            continue;
        FilePath qmakeNow = v->qmakeFilePath();
        int uniqueIdNow = v->uniqueId();
        if (latestVersion == vNow) {
            if (latestQmakeFilePath > qmakeNow)
                continue;
            if (latestQmakeFilePath == qmakeNow && latestUniqueId >= v->uniqueId())
                continue;
        }
        latestVersion = vNow;
        latestQmlls = qmllsNow;
        latestQmakeFilePath = qmakeNow;
        latestUniqueId = uniqueIdNow;
    }
    return std::make_pair(latestQmlls, latestVersion);
}

static CommandLine commandLineForQmlls(Project *project)
{
    const auto *qtVersion = qtVersionFromProject(project);
    QTC_ASSERT(qtVersion, return {});

    auto [executable, version]
        = qmllsSettings()->m_useLatestQmlls
              ? evaluateLatestQmlls()
              : std::make_pair(qtVersion->hostBinPath() / "qmlls", qtVersion->qtVersion());

    CommandLine result{executable, {}};

    if (auto *configuration = project->activeTarget()->activeBuildConfiguration())
        result.addArgs({"-b", configuration->buildDirectory().path()});

    // qmlls 6.8 and later require the import path
    if (version >= QVersionNumber(6, 8, 0))
        result.addArgs({"-I", qtVersion->qmlPath().path()});

    // add custom import paths that the embedded codemodel uses too
    const QmlJS::ModelManagerInterface::ProjectInfo projectInfo
        = QmlJS::ModelManagerInterface::instance()->projectInfo(project);
    for (QmlJS::PathAndLanguage path : projectInfo.importPaths) {
        if (path.language() == QmlJS::Dialect::Qml)
            result.addArgs({"-I", path.path().path()});
    }

    // qmlls 6.8.1 and later require the documentation path
    if (version >= QVersionNumber(6, 8, 1))
        result.addArgs({"-d", qtVersion->docsPath().path()});

    return result;
}

bool QmllsClientSettings::isValidOnProject(ProjectExplorer::Project *project) const
{
    if (!BaseSettings::isValidOnProject(project))
        return false;

    if (!project || !QtVersionManager::isLoaded())
        return false;

    if (!qtVersionFromProject(project)) {
        Core::MessageManager::writeSilently(
            Tr::tr("Current kit does not have a valid Qt version, disabling QML Language Server..."));
        return false;
    }

    return true;
}

BaseClientInterface *QmllsClientSettings::createInterface(Project *project) const
{
    auto interface = new StdIOClientInterface;
    interface->setCommandLine(commandLineForQmlls(project));
    return interface;
}

Client *QmllsClientSettings::createClient(BaseClientInterface *interface) const
{
    return new QmllsClient(static_cast<StdIOClientInterface *>(interface));
}

class QmllsClientSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmllsClientSettingsWidget(
        const QmllsClientSettings *settings, QWidget *parent = nullptr);

    bool useLatestQmlls() const;
    bool disableBuiltinCodemodel() const;
    bool generateQmllsIniFiles() const;
    bool ignoreMinimumQmllsVersion() const;
    bool useQmllsSemanticHighlighting() const;

private:
    QCheckBox *m_useLatestQmlls;
    QCheckBox *m_disableBuiltinCodemodel;
    QCheckBox *m_generateQmllsIniFiles;
    QCheckBox *m_ignoreMinimumQmllsVersion;
    QCheckBox *m_useQmllsSemanticHighlighting;
};

QWidget *QmllsClientSettings::createSettingsWidget(QWidget *parent) const
{
    return new QmllsClientSettingsWidget(this, parent);
}

bool QmllsClientSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = BaseSettings::applyFromSettingsWidget(widget);

    QmllsClientSettingsWidget *qmllsWidget = qobject_cast<QmllsClientSettingsWidget *>(widget);
    if (!qmllsWidget)
        return changed;

    if (m_useLatestQmlls != qmllsWidget->useLatestQmlls()) {
        m_useLatestQmlls = qmllsWidget->useLatestQmlls();
        changed = true;
    }

    if (m_disableBuiltinCodemodel != qmllsWidget->disableBuiltinCodemodel()) {
        m_disableBuiltinCodemodel = qmllsWidget->disableBuiltinCodemodel();
        changed = true;
    }

    if (m_generateQmllsIniFiles != qmllsWidget->generateQmllsIniFiles()) {
        m_generateQmllsIniFiles = qmllsWidget->generateQmllsIniFiles();
        changed = true;
    }

    if (m_ignoreMinimumQmllsVersion != qmllsWidget->ignoreMinimumQmllsVersion()) {
        m_ignoreMinimumQmllsVersion = qmllsWidget->ignoreMinimumQmllsVersion();
        changed = true;
    }

    if (m_useQmllsSemanticHighlighting != qmllsWidget->useQmllsSemanticHighlighting()) {
        m_useQmllsSemanticHighlighting = qmllsWidget->useQmllsSemanticHighlighting();
        changed = true;
    }

    return changed;
}

void QmllsClientSettings::toMap(Store &map) const
{
    BaseSettings::toMap(map);

    map.insert(useLatestQmllsKey, m_useLatestQmlls);
    map.insert(disableBuiltinCodemodelKey, m_disableBuiltinCodemodel);
    map.insert(generateQmllsIniFilesKey, m_generateQmllsIniFiles);
    map.insert(ignoreMinimumQmllsVersionKey, m_ignoreMinimumQmllsVersion);
    map.insert(useQmllsSemanticHighlightingKey, m_useQmllsSemanticHighlighting);
}

void QmllsClientSettings::fromMap(const Store &map)
{
    BaseSettings::fromMap(map);

    m_useLatestQmlls = map[useLatestQmllsKey].toBool();
    m_disableBuiltinCodemodel = map[disableBuiltinCodemodelKey].toBool();
    m_generateQmllsIniFiles = map[generateQmllsIniFilesKey].toBool();
    m_ignoreMinimumQmllsVersion = map[ignoreMinimumQmllsVersionKey].toBool();
    m_useQmllsSemanticHighlighting = map[useQmllsSemanticHighlightingKey].toBool();
}

bool QmllsClientSettings::isEnabledOnProjectFile(const Utils::FilePath &file) const
{
    Project *project = ProjectManager::projectForFile(file);
    return isEnabledOnProject(project);
}

bool QmllsClientSettings::useQmllsWithBuiltinCodemodelOnProject(const Utils::FilePath &file) const
{
    if (m_disableBuiltinCodemodel)
        return false;

    // disableBuitinCodemodel only makes sense when qmlls is enabled
    Project *project = ProjectManager::projectForFile(file);
    return isEnabledOnProject(project);
}

// first time initialization: port old settings from the QmlJsEditingSettings AspectContainer
static void portFromOldSettings(QmllsClientSettings* qmllsClientSettings)
{
    QtcSettings *settings = BaseAspect::qtcSettings();

    const Key baseKey = Key{QmlJSEditor::Constants::SETTINGS_CATEGORY_QML} + "/";

    auto portSetting = [&settings](const Key &key, bool *output) {
        if (settings->contains(key))
            *output = settings->value(key).toBool();
    };

    constexpr char USE_QMLLS[] = "QmlJSEditor.UseQmlls";
    constexpr char USE_LATEST_QMLLS[] = "QmlJSEditor.UseLatestQmlls";
    constexpr char DISABLE_BUILTIN_CODEMODEL[] = "QmlJSEditor.DisableBuiltinCodemodel";
    constexpr char GENERATE_QMLLS_INI_FILES[] = "QmlJSEditor.GenerateQmllsIniFiles";
    constexpr char IGNORE_MINIMUM_QMLLS_VERSION[] = "QmlJSEditor.IgnoreMinimumQmllsVersion";
    constexpr char USE_QMLLS_SEMANTIC_HIGHLIGHTING[]
        = "QmlJSEditor.EnableQmllsSemanticHighlighting";

    portSetting(baseKey + USE_QMLLS, &qmllsClientSettings->m_enabled);
    portSetting(baseKey + USE_LATEST_QMLLS, &qmllsClientSettings->m_useLatestQmlls);
    portSetting(baseKey + DISABLE_BUILTIN_CODEMODEL, &qmllsClientSettings->m_disableBuiltinCodemodel);
    portSetting(baseKey + GENERATE_QMLLS_INI_FILES, &qmllsClientSettings->m_generateQmllsIniFiles);
    portSetting(baseKey + IGNORE_MINIMUM_QMLLS_VERSION, &qmllsClientSettings->m_ignoreMinimumQmllsVersion);
    portSetting(baseKey + USE_QMLLS_SEMANTIC_HIGHLIGHTING, &qmllsClientSettings->m_useQmllsSemanticHighlighting);
}

void setupQmllsClientSettings()
{
    using namespace LanguageClient;
    QmllsClientSettings *clientSettings = new QmllsClientSettings();

    const ClientType type{
        Constants::QMLLS_CLIENT_SETTINGS_ID,
        clientSettings->m_name,
        []() { return new QmllsClientSettings; },
        false};

    const QList<Utils::Store> savedSettings = LanguageClientSettings::storesBySettingsType(type.id);

    if (!savedSettings.isEmpty())
        clientSettings->fromMap(savedSettings.first());
    else
        portFromOldSettings(clientSettings);

    LanguageClientManager::registerClientSettings(clientSettings);
    LanguageClientSettings::registerClientType(type);
}

QmllsClientSettingsWidget::QmllsClientSettingsWidget(
    const QmllsClientSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_useLatestQmlls(new QCheckBox(Tr::tr("Use from latest Qt version"), this))
    , m_disableBuiltinCodemodel(new QCheckBox(
          Tr::tr("Use advanced features (renaming, find usages, and so on) (experimental)"), this))
    , m_generateQmllsIniFiles(
          new QCheckBox(Tr::tr("Create .qmlls.ini files for new projects"), this))
    , m_ignoreMinimumQmllsVersion(new QCheckBox(
          Tr::tr("Allow versions below Qt %1")
              .arg(QmllsClientSettings::mininumQmllsVersion.toString()),
          this))
    , m_useQmllsSemanticHighlighting(
          new QCheckBox(Tr::tr("Enable semantic highlighting (experimental)"), this))
{
    m_useLatestQmlls->setChecked(settings->m_useLatestQmlls);
    m_disableBuiltinCodemodel->setChecked(settings->m_disableBuiltinCodemodel);
    m_generateQmllsIniFiles->setChecked(settings->m_generateQmllsIniFiles);
    m_ignoreMinimumQmllsVersion->setChecked(settings->m_ignoreMinimumQmllsVersion);
    m_useQmllsSemanticHighlighting->setChecked(settings->m_useQmllsSemanticHighlighting);

    using namespace Layouting;
    // clang-format off
    auto form = Form {
        m_ignoreMinimumQmllsVersion, br,
        m_disableBuiltinCodemodel, br,
        m_useQmllsSemanticHighlighting, br,
        m_useLatestQmlls, br,
        m_generateQmllsIniFiles, br,
    };
    // clang-format on

    form.attachTo(this);
}
bool QmllsClientSettingsWidget::useLatestQmlls() const
{
    return m_useLatestQmlls->isChecked();
}
bool QmllsClientSettingsWidget::disableBuiltinCodemodel() const
{
    return m_disableBuiltinCodemodel->isChecked();
}
bool QmllsClientSettingsWidget::generateQmllsIniFiles() const
{
    return m_generateQmllsIniFiles->isChecked();
}
bool QmllsClientSettingsWidget::ignoreMinimumQmllsVersion() const
{
    return m_ignoreMinimumQmllsVersion->isChecked();
}
bool QmllsClientSettingsWidget::useQmllsSemanticHighlighting() const
{
    return m_useQmllsSemanticHighlighting->isChecked();
}

} // namespace QmlJSEditor

#include "qmllsclientsettings.moc"
