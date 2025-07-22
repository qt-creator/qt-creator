// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "studiowelcomeplugin.h"
#include "examplecheckout.h"

#include "qdsnewdialog.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/projectfilecontenttools.h>
#include <qmlprojectmanager/qmlproject.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <qmldesignerbase/qmldesignerbaseplugin.h>

#include <qmldesigner/components/componentcore/theme.h>
#include <qmldesigner/dynamiclicensecheck.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesigner/qmldesignerplugin.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/appinfo.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/infobar.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <nanotrace/nanotrace.h>

#include <QAbstractListModel>
#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMainWindow>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QQuickWidget>
#include <QSettings>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <memory>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace StudioWelcome {
namespace Internal {

static void openOpenProjectDialog()
{
    const FilePath path = Core::DocumentManager::useProjectsDirectory()
                              ? Core::DocumentManager::projectsDirectory()
                              : FilePath();
    const FilePaths files = Core::DocumentManager::getOpenFileNames("*.qmlproject", path);
    if (!files.isEmpty())
        Core::ICore::openFiles(files, Core::ICore::None);
}

constexpr char LAST_QDS_VERSION_ENTRY[] = "QML/Designer/lastQDSVersion";
const char DO_NOT_SHOW_SPLASHSCREEN_AGAIN_KEY[] = "StudioSplashScreen";

const char TELEMETRY_INSIGHT_SETTING[] = "Telemetry";
const char CRASH_REPORTER_SETTING[] = "CrashReportingEnabled";

QPointer<QQuickView> s_viewWindow = nullptr;
QPointer<QQuickWidget> s_viewWidget = nullptr;
static StudioWelcomePlugin *s_pluginInstance = nullptr;

static Utils::FilePath getMainUiFileWithFallback()
{
    const auto project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return {};

    if (!project->activeTarget())
        return {};

    const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        project->activeTarget()->buildSystem());

    if (!qmlBuildSystem)
        return {};

    return qmlBuildSystem->getStartupQmlFileWithFallback();
}

std::unique_ptr<QSettings> makeUserFeedbackSettings()
{
    QStringList domain = QCoreApplication::organizationDomain().split(QLatin1Char('.'));
    std::reverse(domain.begin(), domain.end());
    QString productId = domain.join('.');
    if (!productId.isEmpty())
        productId += ".";
    productId += QCoreApplication::applicationName();

    QString organization;
    if (Utils::HostOsInfo::isMacHost()) {
        organization = QCoreApplication::organizationDomain().isEmpty()
                           ? QCoreApplication::organizationName()
                           : QCoreApplication::organizationDomain();
    } else {
        organization = QCoreApplication::organizationName().isEmpty()
                           ? QCoreApplication::organizationDomain()
                           : QCoreApplication::organizationName();
    }

    std::unique_ptr<QSettings> settings(new QSettings(organization, "UserFeedback." + productId));
    settings->beginGroup("UserFeedback");
    return settings;
}

class UsageStatisticPluginModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool usageStatisticEnabled MEMBER m_usageStatisticEnabled NOTIFY usageStatisticChanged)
    Q_PROPERTY(bool crashReporterEnabled MEMBER m_crashReporterEnabled NOTIFY crashReporterEnabledChanged)
    Q_PROPERTY(QString version MEMBER m_versionString CONSTANT)

public:
    explicit UsageStatisticPluginModel(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_versionString = Utils::appInfo().displayVersion;
        setupModel();
    }

    void setupModel()
    {
        m_usageStatisticEnabled = Core::ICore::settings()->value(TELEMETRY_INSIGHT_SETTING, false).toBool();
        m_crashReporterEnabled = Core::ICore::settings()->value(CRASH_REPORTER_SETTING, false).toBool();

        emit usageStatisticChanged();
        emit crashReporterEnabledChanged();
    }
    Q_INVOKABLE void setTelemetryEnabled(bool b)
    {
        if (m_usageStatisticEnabled == b && m_crashReporterEnabled == b)
            return;

        ICore::askForRestart(tr("The change will take effect after restart."));

        ICore::settings()->setValue(TELEMETRY_INSIGHT_SETTING, b);
        ICore::settings()->setValue(CRASH_REPORTER_SETTING, b);
    }

signals:
    void usageStatisticChanged();
    void crashReporterEnabledChanged();

private:
    bool m_usageStatisticEnabled = false;
    bool m_crashReporterEnabled = false;
    QString m_versionString;
};

class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { FilePathRole = Qt::UserRole + 1, PrettyFilePathRole, PreviewUrl, TagData, Description };

    Q_PROPERTY(bool communityVersion MEMBER m_communityVersion NOTIFY communityVersionChanged)
    Q_PROPERTY(bool enterpriseVersion MEMBER m_enterpriseVersion NOTIFY enterpriseVersionChanged)
    Q_PROPERTY(bool liteDesignerEnabled MEMBER m_liteDesignerEnabled CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    explicit ProjectModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() { return ProjectExplorer::ProjectExplorerPlugin::recentProjects().count(); }

    Q_INVOKABLE void createProject()
    {
        QTimer::singleShot(0, this, []() {
            ProjectExplorer::ProjectExplorerPlugin::openNewProjectDialog();
        });
    }

    Q_INVOKABLE void openProject()
    {
        QTimer::singleShot(0, this, []() { openOpenProjectDialog(); });
    }

    Q_INVOKABLE void openProjectAt(int row)
    {
        if (m_blockOpenRecent)
            return;

        m_blockOpenRecent = true;
        const FilePath projectFile = FilePath::fromVariant(
            data(index(row, 0), ProjectModel::FilePathRole));
        if (projectFile.exists()) {
            const OpenProjectResult result
                = ProjectExplorer::ProjectExplorerPlugin::openProject(projectFile);
            if (!result && !result.alreadyOpen().isEmpty()) {
                const auto fileToOpen = getMainUiFileWithFallback();
                if (!fileToOpen.isEmpty() && fileToOpen.exists() && !fileToOpen.isDir()) {
                    Core::EditorManager::openEditor(fileToOpen, Utils::Id());
                }
            };
        }

        delayedResetProjects();
    }

    Q_INVOKABLE void removeFromRecentProjects(int row)
    {
        if (m_blockOpenRecent)
            return;

        m_blockOpenRecent = true;
        const FilePath projectFile = FilePath::fromVariant(
            data(index(row, 0), ProjectModel::FilePathRole));

        if (projectFile.exists())
            ProjectExplorer::ProjectExplorerPlugin::removeFromRecentProjects(projectFile);

        resetProjects();
    }

    Q_INVOKABLE void clearRecentProjects()
    {
        if (m_blockOpenRecent)
            return;

        m_blockOpenRecent = true;

        ProjectExplorer::ProjectExplorerPlugin::clearRecentProjects();

        resetProjects();
    }

    Q_INVOKABLE int get(int) { return -1; }

    Q_INVOKABLE void showHelp()
    {
        QDesktopServices::openUrl(
            QUrl("qthelp://org.qt-project.qtdesignstudio/doc/studio-getting-started.html"));
    }

    Q_INVOKABLE void showLink(const QString &link)
    {
        QDesktopServices::openUrl(QUrl::fromUserInput(link));
    }

    Q_INVOKABLE void openExample(const QString &examplePath,
                                 const QString &exampleName,
                                 const QString &,
                                 const QString &explicitQmlproject)
    {
        QTC_ASSERT(!exampleName.isEmpty(), return );
        QmlDesigner::QmlDesignerPlugin::emitUsageStatistics("exampleOpened:"
                                                            + exampleName);

        const QString exampleFolder = examplePath + "/" + exampleName + "/";

        QString projectFile = exampleFolder + exampleName + ".qmlproject";

        if (!explicitQmlproject.isEmpty())
            projectFile = exampleFolder + explicitQmlproject;

        ProjectExplorer::ProjectExplorerPlugin::openProjectWelcomePage(FilePath::fromString(projectFile));
    }

    Q_INVOKABLE void openExample(const QString &example,
                                 const QString &formFile,
                                 const QString &url,
                                 const QString &explicitQmlproject,
                                 const QString &tempFile,
                                 const QString &completeBaseName)
    {
        Q_UNUSED(url)
        Q_UNUSED(explicitQmlproject)
        Q_UNUSED(tempFile)
        Q_UNUSED(completeBaseName)
        const FilePath projectFile = Core::ICore::resourcePath("examples")
                / example / (example + ".qmlproject");
        ProjectExplorer::ProjectExplorerPlugin::openProjectWelcomePage(projectFile);
        const FilePath qmlFile = Core::ICore::resourcePath("examples")
                / example / formFile;

        Core::EditorManager::openEditor(qmlFile);
    }

    Q_INVOKABLE bool exampleVersionOk(const QString &exampleVersion)
    {
        if (exampleVersion.isEmpty())
            return true;

        // Split versions into parts (major, minor, patch)
        QStringList qdsVersionParts = QCoreApplication::applicationVersion().split('.');
        QStringList exampleVersionParts = exampleVersion.split('.');

        // Fill missing parts with zeros
        while (qdsVersionParts.size() < 3)
            qdsVersionParts.append("0");

        while (exampleVersionParts.size() < 3)
            exampleVersionParts.append("0");

        int qdsMajor = qdsVersionParts.at(0).toInt();
        int qdsMinor = qdsVersionParts.at(1).toInt();
        int qdsPatch = qdsVersionParts.at(2).toInt();

        int exMajor = exampleVersionParts.at(0).toInt();
        int exMinor = exampleVersionParts.at(1).toInt();
        int exPatch = exampleVersionParts.at(2).toInt();

        return QT_VERSION_CHECK(exMajor, exMinor, exPatch)
               <= QT_VERSION_CHECK(qdsMajor, qdsMinor, qdsPatch);
    }

public slots:
    void resetProjects();
    void delayedResetProjects();

signals:
    void communityVersionChanged();
    void enterpriseVersionChanged();
    void countChanged();

private:
    void setupVersion();

    bool m_communityVersion = true;
    bool m_enterpriseVersion = false;
    bool m_blockOpenRecent = false;
    bool m_liteDesignerEnabled = false;
};

void ProjectModel::setupVersion()
{
    QmlDesigner::FoundLicense license = QmlDesigner::checkLicense();
    m_communityVersion = license == QmlDesigner::FoundLicense::community;
    m_enterpriseVersion = license == QmlDesigner::FoundLicense::enterprise;
    m_liteDesignerEnabled = QmlDesigner::QmlDesignerBasePlugin::isLiteModeEnabled();
}

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            &ProjectExplorer::ProjectExplorerPlugin::recentProjectsChanged,
            this,
            &ProjectModel::delayedResetProjects);

    connect(this, &QAbstractListModel::modelReset, this, &ProjectModel::countChanged);

    setupVersion();
}

int ProjectModel::rowCount(const QModelIndex &) const
{
    return ProjectExplorer::ProjectExplorerPlugin::recentProjects().count();
}

static QString getQDSVersion(const FilePath &projectFilePath)
{
    const QString qdsVersion = QmlProjectManager::ProjectFileContentTools::qdsVersion(
                                        projectFilePath);

    return ProjectModel::tr("Created with Qt Design Studio version: %1").arg(qdsVersion);
}

static QString fromCamelCase(const QString &s) {

   const QRegularExpression regExp1 {"(.)([A-Z][a-z]+)"};
   const QRegularExpression regExp2 {"([a-z0-9])([A-Z])"};
   QString result = s;
   result.replace(regExp1, "\\1 \\2");
   result.replace(regExp2, "\\1 \\2");
   result = result.left(1).toUpper() + result.mid(1);
   return result;
}

static QString resolutionFromConstants(const FilePath &projectFilePath)
{
    QmlProjectManager::ProjectFileContentTools::Resolution res =
            QmlProjectManager::ProjectFileContentTools::resolutionFromConstants(
                projectFilePath);

    if (res.width > 0 && res.height > 0)
        return ProjectModel::tr("Resolution: %1x%2").arg(res.width).arg(res.height);

    return {};
}

static QString description(const FilePath &projectFilePath)
{

    const QString created = ProjectModel::tr("Created: %1").arg(
            projectFilePath.toFileInfo().fileTime(QFileDevice::FileBirthTime).toString());
    const QString lastEdited =  ProjectModel::tr("Last Edited: %1").arg(
            projectFilePath.toFileInfo().fileTime(QFileDevice::FileModificationTime).toString());

    return fromCamelCase(projectFilePath.baseName()) + "\n\n" + created + "\n" + lastEdited
            + "\n" + resolutionFromConstants(projectFilePath)
            + "\n" + getQDSVersion(projectFilePath);
}

static QString tags(const FilePath &projectFilePath)
{
    const Result<QByteArray> res = projectFilePath.fileContents();
    if (!res) {
        // default return
        return "content/App.qml";
    }

    const QByteArray data = *res;

    const bool isQt6 = data.contains("qt6Project: true");
    const bool isMcu = data.contains("qtForMCUs: true");

    if (isMcu)
        return "Qt For MCU";
    if (isQt6)
        return "Qt 6";
    return "Qt 5";
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() ||
        index.row() >= ProjectExplorer::ProjectExplorerPlugin::recentProjects().count()) {

        return {};
    }

    const ProjectExplorer::RecentProjectsEntry data =
            ProjectExplorer::ProjectExplorerPlugin::recentProjects().at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.displayName;
        break;
    case FilePathRole:
        return data.filePath.toVariant();
    case PrettyFilePathRole:
        if (Utils::HostOsInfo::isWindowsHost())
            return data.filePath.absolutePath().nativePath();
        else
            return data.filePath.absolutePath().withTildeHomePath();
    case PreviewUrl:
        return QVariant(
            QStringLiteral("image://project_preview/")
            + QmlProjectManager::ProjectFileContentTools::appQmlFile(data.filePath));
    case TagData:
        return tags(data.filePath);
    case Description:
        return description(data.filePath);
    default:
        return QVariant();
    }

    return QVariant();
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "displayName";
    roleNames[FilePathRole] = "filePath";
    roleNames[PrettyFilePathRole] = "prettyFilePath";
    roleNames[PreviewUrl] = "previewUrl";
    roleNames[TagData] = "tagData";
    roleNames[Description] = "description";
    return roleNames;
}

void ProjectModel::resetProjects()
{
    beginResetModel();
    endResetModel();
    m_blockOpenRecent = false;
}

void ProjectModel::delayedResetProjects()
{
    QTimer::singleShot(2000, this, [this] {
        beginResetModel();
        endResetModel();
        m_blockOpenRecent = false;
    });
}

class WelcomeMode : public Core::IMode
{
    Q_OBJECT
public:
    WelcomeMode();
    ~WelcomeMode() override;

private:
    void setupQuickWidget(const QString &welcomePagePath);
    void createQuickWidget();

    QQuickWidget *m_quickWidget = nullptr;
    QWidget *m_modeWidget = nullptr;
    DataModelDownloader *m_dataModelDownloader = nullptr;
};

void StudioWelcomePlugin::closeSplashScreen()
{
    Utils::CheckableDecider(DO_NOT_SHOW_SPLASHSCREEN_AGAIN_KEY).doNotAskAgain();
    if (!s_viewWindow.isNull())
        s_viewWindow->deleteLater();

    if (!s_viewWidget.isNull())
        s_viewWidget->deleteLater();
}

StudioWelcomePlugin::StudioWelcomePlugin()
{
    s_pluginInstance = this;
}

StudioWelcomePlugin::~StudioWelcomePlugin()
{
    delete m_welcomeMode;
}

void StudioWelcomePlugin::initialize()
{
    qmlRegisterType<ProjectModel>("projectmodel", 1, 0, "ProjectModel");
    qmlRegisterType<UsageStatisticPluginModel>("usagestatistics", 1, 0, "UsageStatisticModel");

    m_welcomeMode = new WelcomeMode;
}

static bool forceDownLoad()
{
    const Key lastQDSVersionEntry = "QML/Designer/ForceWelcomePageDownload";
    return Core::ICore::settings()->value(lastQDSVersionEntry, false).toBool();
}

static bool isFirstUsage()
{
    static const bool qdsVersionUpdated = []() -> bool {
        Utils::QtcSettings *settings = Core::ICore::settings();

        const QString lastQDSVersion = settings->value(LAST_QDS_VERSION_ENTRY).toString();
        const QString currentVersion = Utils::appInfo().displayVersion;

        if (currentVersion != lastQDSVersion) {
            settings->setValue(LAST_QDS_VERSION_ENTRY, currentVersion);
            return true;
        }
        return false;
    }();

    return qdsVersionUpdated;
}

static bool showSplashScreen()
{
    // some error dialog is maybe open, be silent to avoid focus problems (macOS had some)
    if (Core::ICore::mainWindow() != Core::ICore::dialogParent())
        return false;

    if (isFirstUsage())
        return true;

    return Utils::CheckableDecider(DO_NOT_SHOW_SPLASHSCREEN_AGAIN_KEY).shouldAskAgain();
}

void StudioWelcomePlugin::extensionsInitialized()
{
    Core::ModeManager::activateMode(m_welcomeMode->id());

    // Enable QDS new project dialog and QDS wizards
    if (Core::ICore::isQtDesignStudio()) {
        ProjectExplorer::JsonWizardFactory::setInstalledWizardsPath(
            Core::ICore::resourcePath("qmldesigner/studio_templates"));

        Core::ICore::setNewDialogFactory([](QWidget *parent) { return new QdsNewDialog(parent); });

        const QString filters = QString("Project (*.qmlproject);;UI file (*.ui.qml);;QML file "
                                        "(*.qml);;JavaScript file (*.js);;%1")
                                    .arg(Core::DocumentManager::allFilesFilterString());

        Core::DocumentManager::setFileDialogFilter(filters);
    }

    if (showSplashScreen()) {
        connect(Core::ICore::instance(), &Core::ICore::coreOpened, this, [this] {
            NANOTRACE_SCOPE("StudioWelcome",
                            "StudioWelcomePlugin::extensionsInitialized::coreOpened");
            Core::ModeManager::setModeStyle(Core::ModeManager::Style::Hidden);
            if (Utils::HostOsInfo::isMacHost()) {
                s_viewWindow = new QQuickView(Core::ICore::mainWindow()->windowHandle());

                s_viewWindow->setFlag(Qt::FramelessWindowHint);

                s_viewWindow->engine()->addImportPath("qrc:/studiofonts");
#ifdef QT_DEBUG
                s_viewWindow->engine()->addImportPath(QLatin1String(STUDIO_QML_PATH)
                                                      + "splashscreen/imports");
                s_viewWindow->setSource(
                    QUrl::fromLocalFile(QLatin1String(STUDIO_QML_PATH) + "splashscreen/main.qml"));
#else
                s_viewWindow->engine()->addImportPath("qrc:/qml/splashscreen/imports");
                s_viewWindow->setSource(QUrl("qrc:/qml/splashscreen/main.qml"));
#endif

                QTC_ASSERT(s_viewWindow->rootObject(),
                           qWarning() << "The StudioWelcomePlugin has a runtime depdendency on "
                                         "qt/qtquicktimeline.";
                           return );

                connect(s_viewWindow->rootObject(),
                        SIGNAL(closeClicked()),
                        this,
                        SLOT(closeSplashScreen()));

                auto mainWindow = Core::ICore::mainWindow()->windowHandle();
                s_viewWindow->setPosition((mainWindow->width() - s_viewWindow->width()) / 2,
                                          (mainWindow->height() - s_viewWindow->height()) / 2);

                Core::ICore::mainWindow()->setEnabled(false);
                connect(s_viewWindow, &QObject::destroyed, []() {
                    if (Core::ICore::mainWindow())
                        Core::ICore::mainWindow()->setEnabled(true);
                });

                s_viewWindow->show();
                s_viewWindow->requestActivate();
                s_viewWindow->setObjectName(QmlDesigner::Constants::OBJECT_NAME_SPLASH_SCREEN);
            } else {
                s_viewWidget = new QQuickWidget(Core::ICore::dialogParent());

                s_viewWidget->setWindowFlag(Qt::SplashScreen, true);

                s_viewWidget->setObjectName(QmlDesigner::Constants::OBJECT_NAME_SPLASH_SCREEN);
                s_viewWidget->setWindowModality(Qt::ApplicationModal);
                s_viewWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
                s_viewWidget->engine()->addImportPath("qrc:/studiofonts");
#ifdef QT_DEBUG
                s_viewWidget->engine()->addImportPath(QLatin1String(STUDIO_QML_PATH)
                                                      + "splashscreen/imports");
                s_viewWidget->setSource(
                    QUrl::fromLocalFile(QLatin1String(STUDIO_QML_PATH) + "splashscreen/main.qml"));
#else
                s_viewWidget->engine()->addImportPath("qrc:/qml/splashscreen/imports");
                s_viewWidget->setSource(QUrl("qrc:/qml/splashscreen/main.qml"));
#endif

                QTC_ASSERT(s_viewWidget->rootObject(),
                           qWarning() << "The StudioWelcomePlugin has a runtime depdendency on "
                                         "qt/qtquicktimeline.";
                           return );

                connect(s_viewWidget->rootObject(),
                        SIGNAL(closeClicked()),
                        this,
                        SLOT(closeSplashScreen()));

                s_viewWidget->show();
                s_viewWidget->raise();
                s_viewWidget->setFocus();
            }
        });
    }
}

bool StudioWelcomePlugin::delayedInitialize()
{
    QTimer::singleShot(2000, this, []() {
        if (!ProjectExplorer::KitManager::isLoaded())
            return;
        auto modelManager = QmlJS::ModelManagerInterface::instance();
        if (!modelManager)
            return;

        QmlJS::PathsAndLanguages importPaths;

        const QList<Kit *> kits = Utils::filtered(KitManager::kits(), [](const Kit *k) {
            const QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
            const bool valid = version && version->isValid();
            const bool isQt6 = valid && version->qtVersion().majorVersion() == 6;
            const bool autoDetected = valid && version->detectionSource().isAutoDetected();

            return isQt6 && autoDetected
                   && RunDeviceTypeKitAspect::deviceTypeId(k)
                          == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
        });

        for (const Kit *kit : kits) {
            const QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);

            const Utils::FilePath qmlPath = version->qmlPath();
            importPaths.maybeInsert(qmlPath, QmlJS::Dialect::QmlQtQuick2);

            QmlJS::ModelManagerInterface::importScan(QmlJS::ModelManagerInterface::workingCopy(),
                                                     importPaths,
                                                     modelManager,
                                                     false);
        }
    });

    return true;
}

WelcomeMode::WelcomeMode()
{
    setDisplayName(tr("Welcome"));

    const QString welcomePagePath = Core::ICore::resourcePath("qmldesigner/welcomepage").toUrlishString();

    m_dataModelDownloader = new DataModelDownloader(this);
    if (!m_dataModelDownloader->exists()) { //Fallback if data cannot be downloaded
        // TODO: Check result?
        Utils::FilePath::fromUserInput(welcomePagePath + "/dataImports")
            .copyRecursively(m_dataModelDownloader->targetFolder());

        m_dataModelDownloader->setForceDownload(true);
    }
    Utils::FilePath readme = Utils::FilePath::fromUserInput(m_dataModelDownloader->targetFolder().toUrlishString()
                                                            + "/readme.txt");


    const Utils::Icon FLAT({{":/studiowelcome/images/mode_welcome_mask.png",
                      Utils::Theme::IconsBaseColor}});
    setIcon(Icon::sideBarIcon(FLAT, FLAT));

    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(Core::Constants::MODE_WELCOME);
    setContext(Core::Context(Core::Constants::C_WELCOME_MODE));

    QFontDatabase::addApplicationFont(":/studiofonts/TitilliumWeb-Regular.ttf");
    ExampleCheckout::registerTypes();

    createQuickWidget();

    if (forceDownLoad() || !readme.exists()) // Only downloads contain the readme
        m_dataModelDownloader->setForceDownload(true);

    connect(m_dataModelDownloader, &DataModelDownloader::progressChanged, this, [this](){
        if (m_quickWidget->rootObject())
            m_quickWidget->rootObject()->setProperty("loadingProgress", m_dataModelDownloader->progress());
    });

    connect(m_dataModelDownloader, &DataModelDownloader::finished, this, [this, welcomePagePath]() {
        delete m_quickWidget;
        createQuickWidget();
        setupQuickWidget(welcomePagePath);
        m_modeWidget->layout()->addWidget(m_quickWidget);
    });

    connect(m_dataModelDownloader, &DataModelDownloader::downloadFailed, this, [this] {
        m_quickWidget->setEnabled(true);
    });


    if (m_dataModelDownloader->start())
        m_quickWidget->setEnabled(false);

/*
    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged, this, [this](Utils::Id mode){
       bool active = (mode == Core::Constants::MODE_WELCOME);
       m_modeWidget->rootObject()->setProperty("active", active);
    });
*/
    setupQuickWidget(welcomePagePath);

    QVBoxLayout *boxLayout = new QVBoxLayout();
    boxLayout->setContentsMargins(0, 0, 0, 0);

    m_modeWidget = new QWidget;
    m_modeWidget->setLayout(boxLayout);
    boxLayout->addWidget(m_quickWidget);
    IContext::attach(m_modeWidget, {}, "Qt Design Studio Manual");
    setWidget(m_modeWidget);

    QStringList designStudioQchPathes;
    QDir qchDir(Core::HelpManager::documentationPath());
    for (const QFileInfo &fileInfo : qchDir.entryInfoList({"*.qch"}, QDir::Files))
        designStudioQchPathes.append(fileInfo.absoluteFilePath());

    Core::HelpManager::registerDocumentation(designStudioQchPathes);
}

WelcomeMode::~WelcomeMode()
{
    delete m_modeWidget;
}

void WelcomeMode::setupQuickWidget(const QString &welcomePagePath)
{
    m_quickWidget->rootContext()->setContextProperty("$dataModel", m_dataModelDownloader);
    m_quickWidget->rootContext()
        ->setContextProperty("isFirstUsage", QVariant::fromValue(isFirstUsage()));

    m_quickWidget->engine()->addImportPath(
        Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources/imports").toUrlishString());

    m_quickWidget->engine()->addImportPath(welcomePagePath + "/imports");
    m_quickWidget->engine()->addImportPath(m_dataModelDownloader->targetFolder().toUrlishString());
    m_quickWidget->setSource(QUrl::fromLocalFile(welcomePagePath + "/main.qml"));

    QShortcut *updateShortcut = nullptr;
    if (Utils::HostOsInfo::isMacHost())
        updateShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F5), m_quickWidget);
    else
        updateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5), m_quickWidget);
    connect(updateShortcut, &QShortcut::activated, this, [this, welcomePagePath]() {
        m_quickWidget->setSource(QUrl::fromLocalFile(welcomePagePath + "/main.qml"));
    });
}

void WelcomeMode::createQuickWidget()
{
    m_quickWidget = new QQuickWidget;
    m_quickWidget->setMinimumSize(640, 480);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setObjectName(QmlDesigner::Constants::OBJECT_NAME_WELCOME_PAGE);
    QmlDesigner::Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->engine()->addImportPath("qrc:/studiofonts");

    QmlDesigner::QmlDesignerPlugin::registerPreviewImageProvider(m_quickWidget->engine());

    m_quickWidget->engine()->setOutputWarningsToStandardError(false);
}

} // namespace Internal
} // namespace StudioWelcome

#include "studiowelcomeplugin.moc"
