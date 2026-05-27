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
#include <QLayout>
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
constexpr char extraArgumentsKey[] = "extraArguments";
constexpr char extraArgumentsSelectionKey[] = "extraArgumentsSelection";
constexpr char qmllsProjectSettingsKey[] = "QmllsProjectSettings";

QmllsClientSettings *qmllsSettings()
{
    BaseSettings *qmllsSettings
        = Utils::findOrDefault(LanguageClientManager::currentSettings(), [](BaseSettings *setting) {
              return setting->settingsTypeId() == Constants::QMLLS_CLIENT_SETTINGS_ID;
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
        FileFilter{{"*"}, DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot});
    if (lastVersion.isNull())
        return {};

    path /= lastVersion.toString();
    path /= "qmlls";
    return {path.withExecutableSuffix(), lastVersion};
}

struct QmllsForBuildConfiguration
{
    FilePath executable = {};
    QVersionNumber version = {};
    enum ImportsProjectDirectory { No, Yes } importsProjectDirectory = No;
};

static QmllsForBuildConfiguration evaluateLatestQmlls()
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
    return {latestQmlls, latestVersion};
}

QmllsClientSettings::QmllsClientSettings()
{
    setAutoApply(false);

    name.setValue(Constants::QMLLS_NAME);

    mimeTypes.setValue(supportedMimeTypes());

    settingsTypeId.setValue(Constants::QMLLS_CLIENT_SETTINGS_ID);
    startBehavior.setValue(RequiresProject);
    enabled.setValue(false); // disabled by default
    initializationOptions.setValue("{\"qtCreatorHighlighting\": true}");

    auto latestQmllsDisplay = []() {
        const FilePath executable = evaluateLatestQmlls().executable;
        return executable.isEmpty()
                   ? Tr::tr("Use qmlls from latest Qt")
                   : Tr::tr("Use qmlls from latest Qt (located at \"%1\")").arg(executable.path());
    };
    executableSelection.setSettingsKey(executableSelectionKey);
    executableSelection.addOption(Tr::tr("Use qmlls from project Qt kit"));
    executableSelection.addOption(latestQmllsDisplay());
    executableSelection.addOption(Tr::tr("Use custom qmlls executable"));
    executableSelection.setDisplayStyle(SelectionAspect::DisplayStyle::RadioButtons);
    executableSelection.setDefaultValue(FromQtKit);
    auto updateFromLatestQtText = [this, latestQmllsDisplay]() {
        if (std::optional<SelectionAspect::Option> option = executableSelection.optionForIndex(
                FromLatestQtKit)) {
            option->displayName = latestQmllsDisplay();
            executableSelection.setOptionForIndex(FromLatestQtKit, *option);
        }
    };
    connect(
        QtVersionManager::instance(),
        &QtVersionManager::qtVersionsChanged,
        this,
        updateFromLatestQtText);

    ignoreMinimumQmllsVersion.setLabel(
        Tr::tr("Allow versions below Qt %1")
            .arg(QmllsClientSettings::mininumQmllsVersion.toString()));
    ignoreMinimumQmllsVersion.setSettingsKey(ignoreMinimumQmllsVersionKey);
    ignoreMinimumQmllsVersion.setDefaultValue(false);

    useQmllsSemanticHighlighting.setLabel(Tr::tr("Enable qmlls semantic highlighting"));
    useQmllsSemanticHighlighting.setSettingsKey(useQmllsSemanticHighlightingKey);
    useQmllsSemanticHighlighting.setDefaultValue(false);

    disableBuiltinCodemodel.setLabel(Tr::tr("Use advanced features (Document Outline)"));
    disableBuiltinCodemodel.setSettingsKey(disableBuiltinCodemodelKey);
    disableBuiltinCodemodel.setDefaultValue(false);

    generateQmllsIniFiles.setLabel(Tr::tr("Create .qmlls.ini files for new projects targeting Qt < 6.10"));
    generateQmllsIniFiles.setSettingsKey(generateQmllsIniFilesKey);
    generateQmllsIniFiles.setDefaultValue(false);

    enableCMakeBuilds.setLabel(Tr::tr("Enable qmlls's CMake integration"));
    enableCMakeBuilds.setSettingsKey(enableCMakeBuildsKey);
    enableCMakeBuilds.setDefaultValue(true);

    executable.setSettingsKey(executableKey);
    executable.setExpectedKind(Utils::PathChooser::File);
    executable.setHistoryCompleter("Qmlls.Executable.History");

    extraArguments.setLabelText(Tr::tr("Extra arguments:"));
    extraArguments.setSettingsKey(extraArgumentsKey);
    extraArguments.setHistoryCompleter("Qmlls.ExtraArguments.History");
    extraArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
}

// Estimates the version of qmlls to avoid passing unknown options to qmlls in
// commandLineForQmlls().
static QVersionNumber estimateVersionOfOverridenQmlls(const FilePath &executable)
{
    if (!executable.exists()) {
        Core::MessageManager::writeFlashing(
            Tr::tr("Custom qmlls executable \"%1\" does not exist and was disabled.")
                .arg(qmllsSettings()->executable().path()));
        return {};
    }
    Process qmlls;
    // qmlls versions < 6.9 don't have --version, so search their --help instead
    qmlls.setCommand({executable, {"--help"}});

    // standard output is empty on windows when the GUI message box is not disabled
    Environment qmllsEnvironment = Environment::systemEnvironment();
    qmllsEnvironment.set("QT_COMMAND_LINE_PARSER_NO_GUI_MESSAGE_BOXES", "1");
    qmlls.setEnvironment(qmllsEnvironment);

    qmlls.start();
    qmlls.waitForFinished();
    if (qmlls.exitStatus() != QProcess::NormalExit || qmlls.exitCode() != EXIT_SUCCESS) {
        Core::MessageManager::writeFlashing(
            Tr::tr(
                "Custom qmlls executable \"%1\" exited abnormally and was disabled. The custom "
                "executable output "
                "was:\n%2")
                .arg(qmllsSettings()->executable().path(), qmlls.readAllStandardError()));
        return {};
    }

    const QString output = qmlls.readAllStandardOutput();

    if (!output.contains("qmlls")) {
        Core::MessageManager::writeFlashing(
            Tr::tr(
                "Custom qmlls executable \"%1\" does not seem to be a qmlls executable and was "
                "disabled.")
                .arg(qmllsSettings()->executable().path()));
        return {};
    }

    if (output.contains("-d"))
        return QVersionNumber(6, 8, 1);

    if (output.contains("-I"))
        return QVersionNumber(6, 8, 0);

    // fallback
    return QVersionNumber(6, 5, 0);
}

// uses a "soft" dependency to the python plugin
static FilePath findPythonPath(const BuildConfiguration *bc)
{
    Store map;
    bc->toMap(map);
    auto it = map.find("python");
    return it != map.end() ? FilePath::fromSettings(*it) : FilePath{};
}

static FilePath findPySideQmlls(const BuildConfiguration *bc)
{
    FilePath pythonPath = findPythonPath(bc);
    if (pythonPath.isEmpty())
        return pythonPath;

    return pythonPath.withNewFileName("pyside6-qmlls").withExecutableSuffix();
}

static FilePath findPySideImportPath(const BuildConfiguration *bc)
{
    FilePath pythonPath = findPythonPath(bc);
    if (pythonPath.isEmpty())
        return pythonPath;

    Process queryImportPath;
    queryImportPath.setCommand(
        {pythonPath,
         {"-c",
          R"(from pathlib import Path
import PySide6 as ref_mod
print(Path(ref_mod.__file__).resolve().parent / "Qt" / "qml"))"}});

    queryImportPath.start();
    queryImportPath.waitForFinished();

    if (queryImportPath.exitStatus() != QProcess::NormalExit
        || queryImportPath.exitCode() != EXIT_SUCCESS) {
        Core::MessageManager::writeFlashing(Tr::tr("No PySide import paths were found."));
        return {};
    }
    QString path = queryImportPath.allOutput().trimmed();
    return FilePath::fromUserInput(path);
}

static QmllsForBuildConfiguration evaluateQmlls(const BuildConfiguration *bc)
{
    switch (qmllsSettings()->executableSelection()) {
    case QmllsClientSettings::FromQtKit: {
        if (FilePath executable = findPySideQmlls(bc); executable.exists())
            return {
                executable,
                estimateVersionOfOverridenQmlls(executable),
                QmllsForBuildConfiguration::ImportsProjectDirectory::Yes};
        if (!QtVersionManager::isLoaded())
            return {};
        const QtVersion *qtVersion = QtKitAspect::qtVersion(bc->kit());
        if (!qtVersion)
            return {};
        return {
            QmlJS::ModelManagerInterface::qmllsForBinPath(
                qtVersion->hostBinPath(), qtVersion->qtVersion()),
            qtVersion->qtVersion()};
    }
    case QmllsClientSettings::FromLatestQtKit:
        return evaluateLatestQmlls();
    case QmllsClientSettings::FromUser:
        return {
            qmllsSettings()->executable(),
            estimateVersionOfOverridenQmlls(qmllsSettings()->executable())};
    }
    QTC_ASSERT(false, return {});
}

class QmllsClientProjectSettings : public AspectContainer
{
public:
    enum OverrideSelection { UseGlobalSettings, IgnoreGlobalSettings, ExtendGlobalSettings };

    QmllsClientProjectSettings(ProjectExplorer::Project *project);

    void save(ProjectExplorer::Project *project);

    StringAspect extraArguments{this};
    TypedSelectionAspect<OverrideSelection> extraArgumentsSelection{this};
};

QmllsClientProjectSettings::QmllsClientProjectSettings(ProjectExplorer::Project *project)
{
    extraArguments.setLabelText(Tr::tr("Extra arguments:"));
    extraArguments.setHistoryCompleter("Qmlls.ExtraArguments.History");
    extraArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    extraArguments.setSettingsKey(extraArgumentsKey);

    extraArgumentsSelection.setSettingsKey(extraArgumentsSelectionKey);
    extraArgumentsSelection.addOption(Tr::tr("Use Global Settings"));
    extraArgumentsSelection.addOption(Tr::tr("Override Global Settings"));
    extraArgumentsSelection.addOption(Tr::tr("Extend Global Settings"));
    extraArgumentsSelection.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    extraArgumentsSelection.setDefaultValue(UseGlobalSettings);

    auto updateExtraArgumentEnabledState = [this] {
        extraArguments.setEnabled(extraArgumentsSelection.volatileValue() != UseGlobalSettings);
    };
    updateExtraArgumentEnabledState();
    extraArgumentsSelection.addOnVolatileValueChanged(this, updateExtraArgumentEnabledState);

    Store map = storeFromVariant(project->namedSettings(qmllsProjectSettingsKey));
    fromMap(map);

    addOnChanged(this, [this, project] { save(project); });
}

void QmllsClientProjectSettings::save(ProjectExplorer::Project *project)
{
    Store map;
    toMap(map);
    project->setNamedSettings(qmllsProjectSettingsKey, variantFromStore(map));

    LanguageClientManager::applySettings(qmllsSettings());
}

static CommandLine commandLineForQmlls(const BuildConfiguration *bc)
{
    QmllsForBuildConfiguration qmlls = evaluateQmlls(bc);

    CommandLine result{qmlls.executable, {}};

    const QmllsClientProjectSettings projectSettings(bc->project());
    const QmllsClientProjectSettings::OverrideSelection projectArgumentSelection
        = projectSettings.extraArgumentsSelection();
    if (projectArgumentSelection != QmllsClientProjectSettings::IgnoreGlobalSettings) {
        result.addArgs(
            ProcessArgs::splitArgs(
                bc->macroExpander()->expand(qmllsSettings()->extraArguments()),
                qmlls.executable.osType()));
    }
    if (projectArgumentSelection != QmllsClientProjectSettings::UseGlobalSettings) {
        result.addArgs(
            ProcessArgs::splitArgs(
                bc->macroExpander()->expand(projectSettings.extraArguments()),
                qmlls.executable.osType()));
    }

    const QString buildDirectory = bc->buildDirectory().path();
    if (!buildDirectory.isEmpty())
        result.addArgs({"-b", buildDirectory});

    const QtVersion *qtVersion = QtKitAspect::qtVersion(bc->kit());

    // qmlls 6.8 and later require the import path
    if (qmlls.version >= QVersionNumber(6, 8, 0)) {
        if (qtVersion)
            result.addArgs({"-I", qtVersion->qmlPath().path()});
        if (auto pythonPath = findPySideImportPath(bc); pythonPath.exists())
            result.addArgs({"-I", pythonPath.path()});

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

        if (qmlls.importsProjectDirectory == QmllsForBuildConfiguration::Yes)
            result.addArgs({"-I", bc->project()->projectDirectory().path()});
    }

    // qmlls 6.8.1 and later require the documentation path
    if (qtVersion && qmlls.version >= QVersionNumber(6, 8, 1))
        result.addArgs({"-d", qtVersion->docsPath().path()});

    if (!qmllsSettings()->enableCMakeBuilds())
        result.addArg("--no-cmake-calls");

    return result;
}

void QmllsClientSettings::attachProjectSpecificSettingsToLayout(
    Project *project, QLayout *parent) const
{
    auto projectSettings = new QmllsClientProjectSettings{project};

    using namespace Layouting;
    // clang-format off
    Layouting::Group group{
        title(Tr::tr("QML Language Server Settings")),
        Column {
            Row {
                projectSettings->extraArgumentsSelection, projectSettings->extraArguments,
            }
        }
    };
    // clang-format on
    parent->addWidget(group.emerge());
}

bool QmllsClientSettings::isValidOnBuildConfiguration(BuildConfiguration *bc) const
{
    if (!BaseSettings::isValidOnBuildConfiguration(bc))
        return false;

    if (!bc)
        return false;

    const auto qmlls = evaluateQmlls(bc);

    if (qmlls.executable.isEmpty() || qmlls.version.isNull()) {
        Core::MessageManager::writeSilently(
            Tr::tr("Current kit does not have a valid Qt version, disabling QML Language Server."));
        return false;
    }
    if (!ignoreMinimumQmllsVersion() && qmlls.version < QmllsClientSettings::mininumQmllsVersion)
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

void QmllsClientSettings::fromMap(const Store &map)
{
    BaseSettings::fromMap(map);
    mimeTypes.setValue(supportedMimeTypes());

    // port from previous settings
    if (map.contains(useLatestQmllsKey))
        executableSelection.setValue(map[useLatestQmllsKey].toBool() ? FromLatestQtKit : FromQtKit);

    // don't overwrite ported settings if the new key is not in use yet.
    if (map.contains(executableSelectionKey)) {
        executableSelection.setValue(static_cast<ExecutableSelection>(
            map[executableSelectionKey].toInt()));
    }
}

bool QmllsClientSettings::isEnabledOnProjectFile(const Utils::FilePath &file) const
{
    Project *project = ProjectManager::projectForFile(file);
    return isEnabledOnProject(project);
}

bool QmllsClientSettings::useQmllsWithBuiltinCodemodelOnProject(Project *project,
                                                                const FilePath &file) const
{
    if (disableBuiltinCodemodel())
        return false;

    // disableBuitinCodemodel only makes sense when qmlls is enabled
    return project && isEnabledOnProject(project) && project->isKnownFile(file);
}

// first time initialization: port old settings from the QmlJsEditingSettings AspectContainer
static void portFromOldSettings(QmllsClientSettings* qmllsClientSettings)
{
    QtcSettings *settings = &Utils::userSettings();

    const Key baseKey = Key{QmlJSEditor::Constants::SETTINGS_CATEGORY_QML} + "/";

    auto portSetting = [&settings](const Key &key, Utils::BoolAspect &output) {
        if (settings->contains(key))
            output.setValue(settings->value(key).toBool());
    };

    constexpr char USE_QMLLS[] = "QmlJSEditor.UseQmlls";
    constexpr char USE_LATEST_QMLLS[] = "QmlJSEditor.UseLatestQmlls";
    constexpr char DISABLE_BUILTIN_CODEMODEL[] = "QmlJSEditor.DisableBuiltinCodemodel";
    constexpr char GENERATE_QMLLS_INI_FILES[] = "QmlJSEditor.GenerateQmllsIniFiles";
    constexpr char IGNORE_MINIMUM_QMLLS_VERSION[] = "QmlJSEditor.IgnoreMinimumQmllsVersion";
    constexpr char USE_QMLLS_SEMANTIC_HIGHLIGHTING[]
        = "QmlJSEditor.EnableQmllsSemanticHighlighting";

    portSetting(baseKey + USE_QMLLS, qmllsClientSettings->enabled);
    if (settings->contains(baseKey + USE_LATEST_QMLLS))
        qmllsClientSettings->executableSelection.setValue(QmllsClientSettings::FromLatestQtKit);
    portSetting(baseKey + DISABLE_BUILTIN_CODEMODEL, qmllsClientSettings->disableBuiltinCodemodel);
    portSetting(baseKey + GENERATE_QMLLS_INI_FILES, qmllsClientSettings->generateQmllsIniFiles);
    portSetting(baseKey + IGNORE_MINIMUM_QMLLS_VERSION, qmllsClientSettings->ignoreMinimumQmllsVersion);
    portSetting(baseKey + USE_QMLLS_SEMANTIC_HIGHLIGHTING, qmllsClientSettings->useQmllsSemanticHighlighting);
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
            return settings->settingsTypeId() == Constants::QMLLS_CLIENT_SETTINGS_ID;
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

class QmllsClientSettingsWidget : public QWidget
{
public:
    explicit QmllsClientSettingsWidget(QmllsClientSettings *settings, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        using namespace Layouting;
        // clang-format off
        auto form = Column {
            Layouting::Group {
                title(Tr::tr("Options")),
                Form {
                    settings->ignoreMinimumQmllsVersion, br,
                    settings->disableBuiltinCodemodel, br,
                    settings->useQmllsSemanticHighlighting, br,
                    settings->generateQmllsIniFiles, br,
                    settings->enableCMakeBuilds, br,
                    settings->extraArguments, br,
                }
            },
            Layouting::Group {
                title(Tr::tr("Executable selection for qmlls")),
                Column {
                    settings->executableSelection, br,
                    settings->executable, br,
                }
            },
        };
        // clang-format on

        settings->executable.pathChooser()->addButton(
            Tr::tr("Download latest standalone qmlls"), this, [this, settings = QPointer(settings)] {
                m_qmllsDownloader.start({downloadGithubQmlls()}, {}, [settings] {
                    if (!settings)
                        return;
                    if (const Utils::FilePath path = evaluateGithubQmlls().first; !path.isEmpty()) {
                        const_cast<QmllsClientSettings *>(settings.data())
                            ->executable.setVolatileValue(path.toUserOutput());
                    }
                });
            });

        auto updateExecutableEnabled = [settings]() {
            const bool enabled = settings->executableSelection.volatileValue() == QmllsClientSettings::FromUser;
            settings->executable.setEnabled(enabled);
        };
        updateExecutableEnabled();
        connect(&settings->executableSelection, &BaseAspect::volatileValueChanged, this, updateExecutableEnabled);


        form.attachTo(this);
    }

private:
    QSingleTaskTreeRunner m_qmllsDownloader;
};

QWidget *QmllsClientSettings::createSettingsWidget(QWidget *parent)
{
    return new QmllsClientSettingsWidget(this, parent);
}

} // namespace QmlJSEditor
