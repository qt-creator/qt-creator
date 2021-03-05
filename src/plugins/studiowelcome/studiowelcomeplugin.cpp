/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "studiowelcomeplugin.h"
#include "examplecheckout.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/restartdialog.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/checkablemessagebox.h>
#include <utils/icon.h>
#include <utils/infobar.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QAbstractListModel>
#include <QApplication>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QFileInfo>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QQuickWidget>
#include <QTimer>

namespace StudioWelcome {
namespace Internal {

const char DO_NOT_SHOW_SPLASHSCREEN_AGAIN_KEY[] = "StudioSplashScreen";

QPointer<QQuickWidget> s_view = nullptr;
static StudioWelcomePlugin *s_pluginInstance = nullptr;

static bool isUsageStatistic(const ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return false;

    return spec->name().contains("UsageStatistic");
}

ExtensionSystem::PluginSpec *getUsageStatisticPlugin()
{
    const auto plugins = ExtensionSystem::PluginManager::plugins();
    return Utils::findOrDefault(plugins, &isUsageStatistic);
}

class UsageStatisticPluginModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool usageStatisticEnabled MEMBER m_usageStatisticEnabled NOTIFY usageStatisticChanged)
public:
    explicit UsageStatisticPluginModel(QObject *parent = nullptr)
        : QObject(parent)
    {
        setupModel();
    }

    void setupModel()
    {
        auto plugin = getUsageStatisticPlugin();
        if (plugin)
            m_usageStatisticEnabled = plugin->isEnabledBySettings();
        else
            m_usageStatisticEnabled = false;

        emit usageStatisticChanged();
    }

    Q_INVOKABLE void setPluginEnabled(bool b)
    {
        auto plugin = getUsageStatisticPlugin();

        if (!plugin)
            return;

        if (plugin->isEnabledBySettings() == b)
            return;

        plugin->setEnabledBySettings(b);
        ExtensionSystem::PluginManager::writeSettings();

        // pause remove splash timer while dialog is open otherwise splash crashes upon removal
        s_pluginInstance->pauseRemoveSplashTimer();

        const QString restartText = tr("The change will take effect after restart.");
        Core::RestartDialog restartDialog(Core::ICore::dialogParent(), restartText);
        restartDialog.exec();

        s_pluginInstance->resumeRemoveSplashTimer();
        setupModel();
    }

signals:
    void usageStatisticChanged();

private:
    bool m_usageStatisticEnabled = false;
};

class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum { FilePathRole = Qt::UserRole+1, PrettyFilePathRole };

     Q_PROPERTY(bool communityVersion MEMBER m_communityVersion NOTIFY communityVersionChanged)

    explicit ProjectModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void createProject()
    {
        ProjectExplorer::ProjectExplorerPlugin::openNewProjectDialog();
    }

    Q_INVOKABLE void openProject()
    {
        ProjectExplorer::ProjectExplorerPlugin::openOpenProjectDialog();
    }

    Q_INVOKABLE void openProjectAt(int row)
    {
        const QString projectFile = data(index(row, 0),
                                         ProjectModel::FilePathRole).toString();
        ProjectExplorer::ProjectExplorerPlugin::openProjectWelcomePage(projectFile);
    }

    Q_INVOKABLE int get(int)
    {
        return -1;
    }

    Q_INVOKABLE void showHelp()
    {
        QDesktopServices::openUrl(QUrl("qthelp://org.qt-project.qtcreator/doc/index.html"));
    }

    Q_INVOKABLE void openExample(const QString &example, const QString &formFile, const QString &url)
    {
        if (!url.isEmpty()) {
            ExampleCheckout *checkout = new ExampleCheckout;
            checkout->checkoutExample(QUrl::fromUserInput(url));
            connect(checkout,
                    &ExampleCheckout::finishedSucessfully,
                    this,
                    [checkout, this, formFile, example]() {
                        const QString projectFile = checkout->extractionFolder() + "/" + example
                                                    + "/" + example + ".qmlproject";

                        ProjectExplorer::ProjectExplorerPlugin::openProjectWelcomePage(projectFile);
                        const QString qmlFile = checkout->extractionFolder() + "/" + example + "/"
                                                + formFile;

                        Core::EditorManager::openEditor(qmlFile);
                    });
            return;
        }

        const QString projectFile = Core::ICore::resourcePath() + "/examples/" + example + "/"
                                    + example + ".qmlproject";
        ProjectExplorer::ProjectExplorerPlugin::openProjectWelcomePage(projectFile);
        const QString qmlFile = Core::ICore::resourcePath() + "/examples/" + example + "/" + formFile;
        Core::EditorManager::openEditor(qmlFile);
    }
public slots:
    void resetProjects();

signals:
    void communityVersionChanged();

private:
    bool m_communityVersion = false;
};

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            &ProjectExplorer::ProjectExplorerPlugin::recentProjectsChanged,
            this,
            &ProjectModel::resetProjects);


    if (!Utils::findOrDefault(ExtensionSystem::PluginManager::plugins(),
                              Utils::equal(&ExtensionSystem::PluginSpec::name, QString("LicenseChecker"))))
        m_communityVersion = true;
}

int ProjectModel::rowCount(const QModelIndex &) const
{
    return ProjectExplorer::ProjectExplorerPlugin::recentProjects().count();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    QPair<QString,QString> data =
            ProjectExplorer::ProjectExplorerPlugin::recentProjects().at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.second;
        break;
    case FilePathRole:
        return data.first;
    case PrettyFilePathRole:
        return Utils::withTildeHomePath(data.first);
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
    return roleNames;
}

void ProjectModel::resetProjects()
{
    beginResetModel();
    endResetModel();
}

class WelcomeMode : public Core::IMode
{
    Q_OBJECT
public:
    WelcomeMode();
    ~WelcomeMode() override;

private:

    QQuickWidget *m_modeWidget = nullptr;
};

void StudioWelcomePlugin::closeSplashScreen()
{
    if (!s_view.isNull()) {
        const bool doNotShowAgain = s_view->rootObject()->property("doNotShowAgain").toBool();
        if (doNotShowAgain)
            Utils::CheckableMessageBox::doNotAskAgain(Core::ICore::settings(),
                                                      DO_NOT_SHOW_SPLASHSCREEN_AGAIN_KEY);

        s_view->deleteLater();
    }
}

void StudioWelcomePlugin::showSystemSettings()
{
    Core::ICore::infoBar()->removeInfo("WarnCrashReporting");
    Core::ICore::infoBar()->globallySuppressInfo("WarnCrashReporting");

    // pause remove splash timer while settings dialog is open otherwise splash crashes upon removal
    pauseRemoveSplashTimer();
    Core::ICore::showOptionsDialog(Core::Constants::SETTINGS_ID_SYSTEM);
    resumeRemoveSplashTimer();
}

StudioWelcomePlugin::StudioWelcomePlugin()
{
    s_pluginInstance = this;
}

StudioWelcomePlugin::~StudioWelcomePlugin()
{
    delete m_welcomeMode;
}

bool StudioWelcomePlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    qmlRegisterType<ProjectModel>("projectmodel", 1, 0, "ProjectModel");
    qmlRegisterType<UsageStatisticPluginModel>("usagestatistics", 1, 0, "UsageStatisticModel");

    m_welcomeMode = new WelcomeMode;

    QFontDatabase fonts;
    QFontDatabase::addApplicationFont(":/studiofonts/TitilliumWeb-Regular.ttf");
    QFont systemFont("Titillium Web", QApplication::font().pointSize());
    QApplication::setFont(systemFont);

    m_removeSplashTimer.setSingleShot(true);
    m_removeSplashTimer.setInterval(15000);
    connect(&m_removeSplashTimer, &QTimer::timeout, this, [this] { closeSplashScreen(); });
    return true;
}

void StudioWelcomePlugin::extensionsInitialized()
{
    Core::ModeManager::activateMode(m_welcomeMode->id());
    if (Utils::CheckableMessageBox::shouldAskAgain(Core::ICore::settings(),
                                                   DO_NOT_SHOW_SPLASHSCREEN_AGAIN_KEY)) {
        connect(Core::ICore::instance(), &Core::ICore::coreOpened, this, [this] {
            s_view = new QQuickWidget(Core::ICore::dialogParent());
            s_view->setResizeMode(QQuickWidget::SizeRootObjectToView);
            s_view->setWindowFlag(Qt::SplashScreen, true);
            s_view->setWindowModality(Qt::ApplicationModal);
            s_view->engine()->addImportPath("qrc:/studiofonts");
        #ifdef QT_DEBUG
            s_view->engine()->addImportPath(QLatin1String(STUDIO_QML_PATH)
                                            + "splashscreen/imports");
            s_view->setSource(QUrl::fromLocalFile(QLatin1String(STUDIO_QML_PATH)
                                          + "splashscreen/main.qml"));
        #else
            s_view->engine()->addImportPath("qrc:/qml/splashscreen/imports");
            s_view->setSource(QUrl("qrc:/qml/splashscreen/main.qml"));
        #endif

            QTC_ASSERT(s_view->rootObject(),
                       qWarning() << "The StudioWelcomePlugin has a runtime depdendency on qt/qtquicktimeline.";
                       return);

            connect(s_view->rootObject(), SIGNAL(closeClicked()), this, SLOT(closeSplashScreen()));
            connect(s_view->rootObject(), SIGNAL(configureClicked()), this, SLOT(showSystemSettings()));

            s_view->show();
            s_view->raise();

            m_removeSplashTimer.start();
        });
    }
}

bool StudioWelcomePlugin::delayedInitialize()
{
    if (s_view.isNull())
        return false;

    QTC_ASSERT(s_view->rootObject() , return true);

#ifdef ENABLE_CRASHPAD
    const bool crashReportingEnabled = true;
    const bool crashReportingOn = Core::ICore::settings()->value("CrashReportingEnabled", false).toBool();
#else
    const bool crashReportingEnabled = false;
    const bool crashReportingOn = false;
#endif

    QMetaObject::invokeMethod(s_view->rootObject(), "onPluginInitialized",
            Q_ARG(bool, crashReportingEnabled), Q_ARG(bool, crashReportingOn));

    return false;
}

void StudioWelcomePlugin::pauseRemoveSplashTimer()
{
    if (m_removeSplashTimer.isActive()) {
        m_removeSplashRemainingTime = m_removeSplashTimer.remainingTime(); // milliseconds
        m_removeSplashTimer.stop();
    }
}

void StudioWelcomePlugin::resumeRemoveSplashTimer()
{
    if (!m_removeSplashTimer.isActive())
        m_removeSplashTimer.start(m_removeSplashRemainingTime);
}

WelcomeMode::WelcomeMode()
{
    setDisplayName(tr("Studio"));

    const Utils::Icon FLAT({{":/studiowelcome/images/mode_welcome_mask.png",
                      Utils::Theme::IconsBaseColor}});
    const Utils::Icon FLAT_ACTIVE({{":/studiowelcome/images/mode_welcome_mask.png",
                             Utils::Theme::IconsModeWelcomeActiveColor}});
    setIcon(Utils::Icon::modeIcon(FLAT, FLAT, FLAT_ACTIVE));

    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(Core::Constants::MODE_WELCOME);
    setContextHelp("Qt Design Studio Manual");
    setContext(Core::Context(Core::Constants::C_WELCOME_MODE));

    m_modeWidget = new QQuickWidget;
    m_modeWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_modeWidget->engine()->addImportPath("qrc:/studiofonts");
#ifdef QT_DEBUG
    m_modeWidget->engine()->addImportPath(QLatin1String(STUDIO_QML_PATH)
                                    + "welcomepage/imports");
    m_modeWidget->setSource(QUrl::fromLocalFile(QLatin1String(STUDIO_QML_PATH)
                                  + "welcomepage/main.qml"));
#else
    m_modeWidget->engine()->addImportPath("qrc:/qml/welcomepage/imports");
    m_modeWidget->setSource(QUrl("qrc:/qml/welcomepage/main.qml"));
#endif

    setWidget(m_modeWidget);

    QStringList designStudioQchPathes = {Core::HelpManager::documentationPath()
                                         + "/qtdesignstudio.qch",
                                         Core::HelpManager::documentationPath() + "/qtquick.qch",
                                         Core::HelpManager::documentationPath()
                                         + "/qtquickcontrols.qch"};

    Core::HelpManager::registerDocumentation(
                Utils::filtered(designStudioQchPathes,
                                [](const QString &path) { return QFileInfo::exists(path); }));
}

WelcomeMode::~WelcomeMode()
{
    delete m_modeWidget;
}

} // namespace Internal
} // namespace StudioWelcome

#include "studiowelcomeplugin.moc"
