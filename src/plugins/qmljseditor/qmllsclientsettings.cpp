// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllsclientsettings.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"
#include "qmllsclient.h"

#include <utils/mimeconstants.h>
#include <utils/networkaccessmanager.h>
#include <utils/progressdialog.h>
#include <utils/qtcsettings.h>
#include <utils/unarchiver.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QCheckBox>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardPaths>
#include <QTimer>

using namespace LanguageClient;
using namespace QtSupport;
using namespace Utils;
using namespace ProjectExplorer;
using namespace Qt::StringLiterals;
using namespace QtTaskTree;

namespace QmlJSEditor {

constexpr char useLatestQmllsKey[] = "useLatestQmlls";
constexpr char executableSelectionKey[] = "executableSelection";
constexpr char disableBuiltinCodemodelKey[] = "disableBuiltinCodemodel";
constexpr char generateQmllsIniFilesKey[] = "generateQmllsIniFiles";
constexpr char enableCMakeBuildsKey[] = "enableCMakeBuilds";
constexpr char ignoreMinimumQmllsVersionKey[] = "ignoreMinimumQmllsVersion";
constexpr char useQmllsSemanticHighlightingKey[] = "enableQmllsSemanticHighlighting";
constexpr char executableKey[] = "executable";

QmllsClientSettings *qmllsSettings()
{
    BaseSettings *qmllsSettings
        = Utils::findOrDefault(LanguageClientManager::currentSettings(), [](BaseSettings *setting) {
              return setting->m_settingsTypeId == Constants::QMLLS_CLIENT_SETTINGS_ID;
          });
    return static_cast<QmllsClientSettings *>(qmllsSettings);
}

static const QStringList &supportedMimeTypes()
{
    using namespace Utils::Constants;
    static const QStringList mimeTypes
        = {QML_MIMETYPE,
           QMLUI_MIMETYPE,
           QMLPROJECT_MIMETYPE,
           QMLTYPES_MIMETYPE,
           JS_MIMETYPE};
    return mimeTypes;
}

static FilePath downloadedQmllsPath()
{
    return FilePath::fromUserInput(
               QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
           / "standalone-qmlls";
}

static std::pair<FilePath, QVersionNumber> evaluateGithubQmlls()
{
    QVersionNumber lastVersion;
    FilePath path = downloadedQmllsPath();
    if (!path.exists())
        return {};

    path.iterateDirectory(
        [&lastVersion](const FilePath &path) {
            if (!path.pathAppended("qmlls").withExecutableSuffix().exists())
                return IterationPolicy::Continue;
            if (auto version = QVersionNumber::fromString(path.fileName());
                !version.isNull() && version > lastVersion) {
                lastVersion = version;
            }
            return IterationPolicy::Continue;
        },
        {{"*"}, QDir::Dirs | QDir::NoDotAndDotDot});
    if (lastVersion.isNull())
        return {};

    path /= lastVersion.toString();
    path /= "qmlls";
    return {path.withExecutableSuffix(), lastVersion};
}

QmllsClientSettings::QmllsClientSettings()
{
    name.setValue(Constants::QMLLS_NAME);

    m_languageFilter.mimeTypes = supportedMimeTypes();

    m_settingsTypeId = Constants::QMLLS_CLIENT_SETTINGS_ID;
    m_startBehavior = RequiresProject;
    m_enabled = false; // disabled by default
    initializationOptions.setValue("{\"qtCreatorHighlighting\": true}");
}

static std::pair<FilePath, QVersionNumber> evaluateLatestQmlls()
{
    if (!QtVersionManager::isLoaded())
        return {};

    const QtVersions versions = QtVersionManager::versions();
    FilePath latestQmlls;
    QVersionNumber latestVersion;
    int latestUniqueId = std::numeric_limits<int>::min();

    for (QtVersion *qtVersion : versions) {
        if (!qtVersion->qmakeFilePath().isLocal())
            continue;

        const QVersionNumber version = qtVersion->qtVersion();
        const int uniqueId = qtVersion->uniqueId();

        // note: break ties between qt kits of same versions by selecting the qt kit with the highest id
        if (std::tie(version, uniqueId) < std::tie(latestVersion, latestUniqueId))
            continue;

        const FilePath qmlls
            = QmlJS::ModelManagerInterface::qmllsForBinPath(qtVersion->hostBinPath(), version);
        if (!qmlls.isExecutableFile())
            continue;

        latestVersion = version;
        latestQmlls = qmlls;
        latestUniqueId = uniqueId;
    }
    return std::make_pair(latestQmlls, latestVersion);
}

// Estimates the version of qmlls to avoid passing unknown options to qmlls in
// commandLineForQmlls().
static QVersionNumber estimateVersionOfOverridenQmlls()
{
    if (!qmllsSettings()->m_executable.exists()) {
        Core::MessageManager::writeFlashing(
                    Tr::tr("Custom qmlls executable \"%1\" does not exist and was disabled.")
                    .arg(qmllsSettings()->m_executable.path()));
        return {};
    }
    Process qmlls;
    // qmlls versions < 6.9 don't have --version, so search their --help instead
    qmlls.setCommand({qmllsSettings()->m_executable, {"--help"}});
    qmlls.start();
    qmlls.waitForFinished();
    if (qmlls.exitStatus() != QProcess::NormalExit || qmlls.exitCode() != EXIT_SUCCESS) {
        Core::MessageManager::writeFlashing(
            Tr::tr(
                "Custom qmlls executable \"%1\" exited abnormally and was disabled. The custom "
                "executable output "
                "was:\n%2")
                .arg(qmllsSettings()->m_executable.path(), qmlls.readAllStandardError()));
        return {};
    }

    const QString output = qmlls.readAllStandardOutput();

    if (!output.contains("qmlls")) {
        Core::MessageManager::writeFlashing(
            Tr::tr(
                "Custom qmlls executable \"%1\" does not seem to be a qmlls executable and was "
                "disabled.")
                .arg(qmllsSettings()->m_executable.path()));
        return {};
    }

    if (output.contains("-d"))
        return QVersionNumber(6, 8, 1);

    if (output.contains("-I"))
        return QVersionNumber(6, 8, 0);

    // fallback
    return QVersionNumber(6, 5, 0);
}

static std::pair<FilePath, QVersionNumber> evaluateQmlls(const QtVersion *qtVersion)
{
    switch (qmllsSettings()->m_executableSelection) {
    case QmllsClientSettings::FromQtKit:
        return std::make_pair(
            QmlJS::ModelManagerInterface::qmllsForBinPath(
                qtVersion->hostBinPath(), qtVersion->qtVersion()),
            qtVersion->qtVersion());
    case QmllsClientSettings::FromLatestQtKit:
        return evaluateLatestQmlls();
    case QmllsClientSettings::FromUser:
        return {qmllsSettings()->m_executable, estimateVersionOfOverridenQmlls()};
    }
    QTC_ASSERT(false, return {});
}

static CommandLine commandLineForQmlls(BuildConfiguration *bc)
{
    const QtVersion *qtVersion = QtKitAspect::qtVersion(bc->kit());
    QTC_ASSERT(qtVersion, return {});

    auto [executable, version] = evaluateQmlls(qtVersion);

    CommandLine result{executable, {}};

    const QString buildDirectory = bc->buildDirectory().path();
    if (!buildDirectory.isEmpty())
        result.addArgs({"-b", buildDirectory});

    // qmlls 6.8 and later require the import path
    if (version >= QVersionNumber(6, 8, 0)) {
        result.addArgs({"-I", qtVersion->qmlPath().path()});

        // add custom import paths that the embedded codemodel uses too
        const QmlJS::ModelManagerInterface::ProjectInfo projectInfo
            = QmlJS::ModelManagerInterface::instance()->projectInfo(bc->project());
        for (QmlJS::PathAndLanguage path : projectInfo.importPaths) {
            if (path.language() == QmlJS::Dialect::Qml)
                result.addArgs({"-I", path.path().path()});
        }

        // work around QTBUG-132263 for qmlls 6.8.2
        if (!buildDirectory.isEmpty())
            result.addArgs({"-I", buildDirectory});
    }

    // qmlls 6.8.1 and later require the documentation path
    if (version >= QVersionNumber(6, 8, 1))
        result.addArgs({"-d", qtVersion->docsPath().path()});

    if (!qmllsSettings()->m_enableCMakeBuilds)
        result.addArg("--no-cmake-calls");

    return result;
}

bool QmllsClientSettings::isValidOnBuildConfiguration(BuildConfiguration *bc) const
{
    if (!BaseSettings::isValidOnBuildConfiguration(bc))
        return false;

    if (!bc || !QtVersionManager::isLoaded())
        return false;

    const QtVersion *qtVersion = QtKitAspect::qtVersion(bc->kit());
    if (!qtVersion) {
        Core::MessageManager::writeSilently(
            Tr::tr("Current kit does not have a valid Qt version, disabling QML Language Server."));
        return false;
    }

    const auto &[filePath, version] = evaluateQmlls(qtVersion);

    if (filePath.isEmpty() || version.isNull())
        return false;
    if (!m_ignoreMinimumQmllsVersion && version < QmllsClientSettings::mininumQmllsVersion)
        return false;

    return true;
}

class QmllsClientInterface : public StdIOClientInterface
{
public:
    FilePath qmllsFilePath() const { return m_cmd.executable(); }
};

BaseClientInterface *QmllsClientSettings::createInterface(BuildConfiguration *bc) const
{
    auto interface = new QmllsClientInterface;
    interface->setCommandLine(commandLineForQmlls(bc));
    return interface;
}

Client *QmllsClientSettings::createClient(BaseClientInterface *interface) const
{
    auto qmllsInterface = static_cast<QmllsClientInterface *>(interface);
    auto client = new QmllsClient(qmllsInterface);
    const QString display = QString("%1 (%2)").arg(
        name(), qmllsInterface->qmllsFilePath().toUserOutput());
    client->setName(display);
    return client;
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
    bool enableCMakeBuilds() const;
    bool ignoreMinimumQmllsVersion() const;
    bool useQmllsSemanticHighlighting() const;
    bool overrideExecutable() const;
    Utils::FilePath executable() const;
    QmllsClientSettings::ExecutableSelection executableSelection() const;

private:
    QCheckBox *m_disableBuiltinCodemodel;
    QCheckBox *m_generateQmllsIniFiles;
    QCheckBox *m_enableCMakeBuilds;
    QCheckBox *m_ignoreMinimumQmllsVersion;
    QCheckBox *m_useQmllsSemanticHighlighting;

    QRadioButton *m_useDefaultQmlls;
    QRadioButton *m_useLatestQmlls;
    QRadioButton *m_overrideExecutable;

    Utils::PathChooser *m_executable;
    QSingleTaskTreeRunner m_qmllsDownloader;
};

QWidget *QmllsClientSettings::createSettingsWidget(QWidget *parent) const
{
    return new QmllsClientSettingsWidget(this, parent);
}

QmllsClientSettings::ExecutableSelection QmllsClientSettingsWidget::executableSelection() const
{
    if (m_useDefaultQmlls->isChecked())
        return QmllsClientSettings::FromQtKit;
    if (m_useLatestQmlls->isChecked())
        return QmllsClientSettings::FromLatestQtKit;
    if (m_overrideExecutable->isChecked())
        return QmllsClientSettings::FromUser;

    QTC_ASSERT(false, return QmllsClientSettings::FromQtKit);
}

bool QmllsClientSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = BaseSettings::applyFromSettingsWidget(widget);

    QmllsClientSettingsWidget *qmllsWidget = qobject_cast<QmllsClientSettingsWidget *>(widget);
    if (!qmllsWidget)
        return changed;

    if (m_executableSelection != qmllsWidget->executableSelection()) {
        m_executableSelection = qmllsWidget->executableSelection();
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

    if (m_enableCMakeBuilds != qmllsWidget->enableCMakeBuilds()) {
        m_enableCMakeBuilds = qmllsWidget->enableCMakeBuilds();
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

    if (m_executable != qmllsWidget->executable()) {
        m_executable = qmllsWidget->executable();
        changed = true;
    }

    return changed;
}

void QmllsClientSettings::toMap(Store &map) const
{
    BaseSettings::toMap(map);

    map.insert(executableSelectionKey, static_cast<int>(m_executableSelection));
    map.insert(disableBuiltinCodemodelKey, m_disableBuiltinCodemodel);
    map.insert(generateQmllsIniFilesKey, m_generateQmllsIniFiles);
    map.insert(enableCMakeBuildsKey, m_enableCMakeBuilds);
    map.insert(ignoreMinimumQmllsVersionKey, m_ignoreMinimumQmllsVersion);
    map.insert(useQmllsSemanticHighlightingKey, m_useQmllsSemanticHighlighting);
    map.insert(executableKey, m_executable.toSettings());
}

void QmllsClientSettings::fromMap(const Store &map)
{
    BaseSettings::fromMap(map);
    m_languageFilter.mimeTypes = supportedMimeTypes();

    // port from previous settings
    if (map.contains(useLatestQmllsKey))
        m_executableSelection = map[useLatestQmllsKey].toBool() ? FromLatestQtKit : FromQtKit;

    // don't overwrite ported settings if the new key is not in use yet.
    if (map.contains(executableSelectionKey))
        m_executableSelection = static_cast<ExecutableSelection>(
            map[executableSelectionKey].toInt());

    m_disableBuiltinCodemodel = map[disableBuiltinCodemodelKey].toBool();
    m_generateQmllsIniFiles = map[generateQmllsIniFilesKey].toBool();
    m_enableCMakeBuilds = map[enableCMakeBuildsKey].toBool();
    m_ignoreMinimumQmllsVersion = map[ignoreMinimumQmllsVersionKey].toBool();
    m_useQmllsSemanticHighlighting = map[useQmllsSemanticHighlightingKey].toBool();
    m_executable = Utils::FilePath::fromSettings(map[executableKey]);
}

bool QmllsClientSettings::isEnabledOnProjectFile(const Utils::FilePath &file) const
{
    Project *project = ProjectManager::projectForFile(file);
    return isEnabledOnProject(project);
}

bool QmllsClientSettings::useQmllsWithBuiltinCodemodelOnProject(Project *project,
                                                                const FilePath &file) const
{
    if (m_disableBuiltinCodemodel)
        return false;

    // disableBuitinCodemodel only makes sense when qmlls is enabled
    return project && isEnabledOnProject(project) && project->isKnownFile(file);
}

// first time initialization: port old settings from the QmlJsEditingSettings AspectContainer
static void portFromOldSettings(QmllsClientSettings* qmllsClientSettings)
{
    QtcSettings *settings = &Utils::userSettings();

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
    if (settings->contains(baseKey + USE_LATEST_QMLLS))
        qmllsClientSettings->m_executableSelection = QmllsClientSettings::FromLatestQtKit;
    portSetting(baseKey + DISABLE_BUILTIN_CODEMODEL, &qmllsClientSettings->m_disableBuiltinCodemodel);
    portSetting(baseKey + GENERATE_QMLLS_INI_FILES, &qmllsClientSettings->m_generateQmllsIniFiles);
    portSetting(baseKey + IGNORE_MINIMUM_QMLLS_VERSION, &qmllsClientSettings->m_ignoreMinimumQmllsVersion);
    portSetting(baseKey + USE_QMLLS_SEMANTIC_HIGHLIGHTING, &qmllsClientSettings->m_useQmllsSemanticHighlighting);
}

void registerQmllsSettings()
{
    const ClientType type{Constants::QMLLS_CLIENT_SETTINGS_ID,
                          Constants::QMLLS_NAME,
                          []() { return new QmllsClientSettings; },
                          false};

    LanguageClientSettings::registerClientType(type);
}

void setupQmllsClient()
{
    if (!Utils::anyOf(LanguageClientManager::currentSettings(), [](const BaseSettings *settings) {
            return settings->m_settingsTypeId == Constants::QMLLS_CLIENT_SETTINGS_ID;
        })) {
        QmllsClientSettings *clientSettings = new QmllsClientSettings();
        portFromOldSettings(clientSettings);
        LanguageClientManager::registerClientSettings(clientSettings);
    }
}

static QString githubQmllsMetadataUrl()
{
    return "https://qtccache.qt.io/QMLLS/LatestRelease";
}

static QString dialogTitle()
{
    return Tr::tr("Download Standalone QML Language Server");
}

static GroupItem downloadGithubQmlls()
{
    struct StorageStruct
    {
        StorageStruct()
        {
            progressDialog.reset(createProgressDialog(
                100, dialogTitle(), Tr::tr("Downloading standalone QML Language Server...")));
        }

        void logWarning(const QString &error)
        {
            progressDialog->close();
            // note: QTimer::singleShot is needed here to avoid spawning QMessageBox::warning()'s nested eventloop during the TaskTree execution
            QTimer::singleShot(0, Core::ICore::dialogParent(), [error] {
                QMessageBox::warning(Core::ICore::dialogParent(), dialogTitle(), error);
            });
        };
        void logInformation(const QString &info)
        {
            progressDialog->close();
            // note: see comment above about need of QTimer::singleShot
            QTimer::singleShot(0, Core::ICore::dialogParent(), [info] {
                QMessageBox::information(Core::ICore::dialogParent(), dialogTitle(), info);
            });
        };
        std::unique_ptr<QProgressDialog> progressDialog;
        std::optional<FilePath> fileName;
        std::optional<QString> downloadUrl;
        FilePath downloadPath;
        std::optional<QVersionNumber> latestVersion;
    };

    const Storage<StorageStruct> storage;

    auto setupNetworkQuery = [storage](QNetworkReplyWrapper *query) {
        QObject::connect(
            query,
            &QNetworkReplyWrapper::downloadProgress,
            storage->progressDialog.get(),
            [storage = storage.activeStorage()](qint64 received, qint64 max) {
                storage->progressDialog->setRange(0, max);
                storage->progressDialog->setValue(received);
            });
#if QT_CONFIG(ssl)
        QObject::connect(
            query,
            &QNetworkReplyWrapper::sslErrors,
            storage->progressDialog.get(),
            [query, storage = storage.activeStorage()](const QList<QSslError> &sslErrors) {
                for (const QSslError &error : sslErrors)
                    Core::MessageManager::writeDisrupting(
                        Tr::tr("SSL error: %1\n").arg(error.errorString()));

                storage->logWarning(Tr::tr("Encountered SSL errors and aborted the download."));
                query->reply()->abort();
            });
#endif
    };

    const auto onMetadataQuerySetup = [storage, setupNetworkQuery](QNetworkReplyWrapper &query) {
        query.setRequest(QNetworkRequest(githubQmllsMetadataUrl()));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        setupNetworkQuery(&query);
    };

    const auto onMetadataQueryDone =
        [storage](const QNetworkReplyWrapper &query, DoneWith result) -> DoneResult {
        if (result == DoneWith::Cancel)
            return DoneResult::Success;

        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return DoneResult::Error);
        const QUrl url = reply->url();
        if (result != DoneWith::Success) {
            storage->logWarning(
                Tr::tr("Downloading from %1 failed: %2.").arg(url.toString(), reply->errorString()));
            return DoneResult::Error;
        }
        QJsonDocument metadata = QJsonDocument::fromJson(reply->readAll());

        if (metadata["tag_name"].isString())
            storage->latestVersion = QVersionNumber::fromString(metadata["tag_name"].toString());

        const OsArch arch = HostOsInfo::binaryArchitecture();
        const bool isArm64 = arch == Utils::OsArchArm64;
        const bool isAmd64 = arch == Utils::OsArchAMD64;
        if (isArm64 || isAmd64) {
            static QLatin1StringView binaryName = [isArm64, isAmd64] {
                if (HostOsInfo::isWindowsHost()) {
                    if (isArm64)
                        return "qmllanguageserver-windows-arm64"_L1;
                    else
                        return "qmllanguageserver-windows-x64"_L1;
                }
                if (HostOsInfo::isMacHost())
                    return "qmllanguageserver-macos"_L1;

                if (isAmd64)
                    return "qmllanguageserver-linux-arm64"_L1;
                return "qmllanguageserver-linux-x64"_L1;
            }();

            for (const auto asset : metadata[u"assets"].toArray()) {
                const QString currentName = asset[u"name"].toString();
                if (currentName.startsWith(binaryName) && !currentName.contains("debug")) {
                    storage->downloadUrl = asset[u"browser_download_url"].toString();
                    return DoneResult::Success;
                }
            }
        }

        storage->logWarning(
            Tr::tr("Could not find a suitable QML Language Server binary for this platform."));
        return DoneResult::Error;
    };

    const auto onQuerySetup = [storage, setupNetworkQuery](QNetworkReplyWrapper &query) {
        if (!storage->latestVersion || !storage->downloadUrl)
            return SetupResult::StopWithError;

        if (auto [qmllsPath, lastVersion] = evaluateGithubQmlls();
            storage->latestVersion <= lastVersion) {
            storage->logWarning(
                Tr::tr("Latest standalone QML Language Server already exists at %1")
                    .arg(qmllsPath.path()));
            return SetupResult::StopWithError;
        }

        const FilePath qmllsDirectory = downloadedQmllsPath() / storage->latestVersion->toString();
        if (qmllsDirectory.exists())
            qmllsDirectory.removeRecursively();
        qmllsDirectory.createDir();
        storage->downloadPath = qmllsDirectory / "archive.zip";

        query.setRequest(QNetworkRequest(*storage->downloadUrl));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        setupNetworkQuery(&query);
        return SetupResult::Continue;
    };
    const auto onQueryDone = [storage](const QNetworkReplyWrapper &query, DoneWith result) -> DoneResult {
        if (result == DoneWith::Cancel)
            return DoneResult::Success;

        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return DoneResult::Error);
        const QUrl url = reply->url();
        if (result != DoneWith::Success) {
            storage->logWarning(
                Tr::tr("Downloading from %1 failed: %2.").arg(url.toString(), reply->errorString()));
            return DoneResult::Error;
        }
        if (const auto result = storage->downloadPath.writeFileContents(reply->readAll()); !result) {
            storage->logWarning(result.error());
            return DoneResult::Error;
        }

        return DoneResult::Success;
    };

    const auto onUnarchiveSetup = [storage](Unarchiver &task) {
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setValue(0);
        storage->progressDialog->setLabelText(Tr::tr("Unarchiving QML Language Server..."));
        task.setArchive(storage->downloadPath);
        task.setDestination(storage->downloadPath.parentDir());
    };
    const auto onUnarchiverDone = [storage](const Unarchiver &task) {
        const Result<> unarchiveResult = task.result();

        if (!unarchiveResult) {
            storage->logWarning(Tr::tr("Unarchiving error: %1").arg(unarchiveResult.error()));
            return;
        }
        const FilePath qmllsDirectory = storage->downloadPath.parentDir();
        const FilePath expectedQmllsLocation = evaluateGithubQmlls().first;
        if (!expectedQmllsLocation.exists() || expectedQmllsLocation.parentDir() != qmllsDirectory) {
            storage->logWarning(
                Tr::tr(
                    "Could not find \"qmlls\" in the extracted archive. Please create a "
                    "bugreport."));
            return;
        }

        storage->logInformation(
            Tr::tr("Standalone QML Language Server succesfully downloaded in %1")
                .arg(qmllsDirectory.path()));
    };

    // clang-format off
    return Group {
        storage,
        QNetworkReplyWrapperTask(onMetadataQuerySetup, onMetadataQueryDone),
        QNetworkReplyWrapperTask(onQuerySetup, onQueryDone),
        UnarchiverTask(onUnarchiveSetup, onUnarchiverDone),
    };
    // clang-format on
}

QmllsClientSettingsWidget::QmllsClientSettingsWidget(
    const QmllsClientSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_disableBuiltinCodemodel(new QCheckBox(
          Tr::tr("Use advanced features (renaming, find usages, and so on) (experimental)"), this))
    , m_generateQmllsIniFiles(
          new QCheckBox(Tr::tr("Create .qmlls.ini files for new projects targeting Qt < 6.10"), this))
    , m_enableCMakeBuilds(new QCheckBox(Tr::tr("Enable qmlls's CMake integration"), this))
    , m_ignoreMinimumQmllsVersion(new QCheckBox(
          Tr::tr("Allow versions below Qt %1")
              .arg(QmllsClientSettings::mininumQmllsVersion.toString()),
          this))
    , m_useQmllsSemanticHighlighting(
          new QCheckBox(Tr::tr("Enable semantic highlighting (experimental)"), this))
    , m_useDefaultQmlls(new QRadioButton(Tr::tr("Use qmlls from project Qt kit"), this))
    , m_useLatestQmlls(new QRadioButton(
          Tr::tr("Use qmlls from latest Qt kit (located at %1)")
              .arg(evaluateLatestQmlls().first.path()),
          this))
    , m_overrideExecutable(new QRadioButton(Tr::tr("Use custom qmlls executable:"), this))
    , m_executable(new Utils::PathChooser(this))
{
    m_useDefaultQmlls->setChecked(settings->m_executableSelection == QmllsClientSettings::FromQtKit);
    m_useLatestQmlls->setChecked(
        settings->m_executableSelection == QmllsClientSettings::FromLatestQtKit);
    m_overrideExecutable->setChecked(
        settings->m_executableSelection == QmllsClientSettings::FromUser);

    m_enableCMakeBuilds->setChecked(settings->m_enableCMakeBuilds);
    m_disableBuiltinCodemodel->setChecked(settings->m_disableBuiltinCodemodel);
    m_generateQmllsIniFiles->setChecked(settings->m_generateQmllsIniFiles);
    m_ignoreMinimumQmllsVersion->setChecked(settings->m_ignoreMinimumQmllsVersion);
    m_useQmllsSemanticHighlighting->setChecked(settings->m_useQmllsSemanticHighlighting);

    QObject::connect(
        m_overrideExecutable, &QCheckBox::toggled, m_executable, &PathChooser::setEnabled);
    m_executable->setFilePath(settings->m_executable);
    m_executable->setExpectedKind(Utils::PathChooser::File);
    m_executable->setEnabled(m_overrideExecutable->isChecked());
    m_executable->setHistoryCompleter("Qmlls.Executable.History");
    m_executable->addButton(Tr::tr("Download latest standalone qmlls"), this, [this] {
        m_qmllsDownloader.start({downloadGithubQmlls()}, {}, [this] {
            if (const Utils::FilePath path = evaluateGithubQmlls().first; !path.isEmpty())
                m_executable->setFilePath(path);
        });
    });

    using namespace Layouting;
    // clang-format off
    auto form = Column {
        Layouting::Group {
            title(Tr::tr("Options")),
            Form {
                m_ignoreMinimumQmllsVersion, br,
                m_disableBuiltinCodemodel, br,
                m_useQmllsSemanticHighlighting, br,
                m_generateQmllsIniFiles, br,
                m_enableCMakeBuilds, br,
            }
        },
        Layouting::Group {
            title(Tr::tr("Executable selection for qmlls")),
            Column {
                Row { m_useDefaultQmlls },
                Row { m_useLatestQmlls },
                Row {
                    Column { m_overrideExecutable },
                    Column { m_executable },
                },
            }
        },
    };
    // clang-format on

    connect(m_useDefaultQmlls, &QCheckBox::toggled, this, markSettingsDirty);
    connect(m_useLatestQmlls, &QCheckBox::toggled, this, markSettingsDirty);
    connect(m_overrideExecutable, &QCheckBox::toggled, this, markSettingsDirty);

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
bool QmllsClientSettingsWidget::enableCMakeBuilds() const
{
    return m_enableCMakeBuilds->isChecked();
}
bool QmllsClientSettingsWidget::ignoreMinimumQmllsVersion() const
{
    return m_ignoreMinimumQmllsVersion->isChecked();
}
bool QmllsClientSettingsWidget::useQmllsSemanticHighlighting() const
{
    return m_useQmllsSemanticHighlighting->isChecked();
}

bool QmllsClientSettingsWidget::overrideExecutable() const
{
    return m_overrideExecutable->isChecked();
}

Utils::FilePath QmllsClientSettingsWidget::executable() const
{
    return m_executable->filePath();
}

} // namespace QmlJSEditor

#include "qmllsclientsettings.moc"
