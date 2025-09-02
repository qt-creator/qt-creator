// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsappsdksettings.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/async.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/pathchooser.h>
#include <utils/progressdialog.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/summarywidget.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QList>
#include <QListWidget>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

using namespace Utils;
using namespace Tasking;

namespace ProjectExplorer::Internal {

static Q_LOGGING_CATEGORY(windowssettingswidget, "qtc.windows.windowssettingswidget", QtWarningMsg);

WindowsAppSdkSettings &windowsAppSdkSettings()
{
    static WindowsAppSdkSettings theWindowsConfigurations;
    return theWindowsConfigurations;
}

WindowsAppSdkSettings::WindowsAppSdkSettings()
{
    setSettingsGroup("WindowsConfigurations");

    downloadLocation.setSettingsKey("DownloadLocation");
    nugetLocation.setSettingsKey("NugetLocation");
    windowsAppSdkLocation.setSettingsKey("WindowsAppSDKLocation");

    AspectContainer::readSettings();

    if (downloadLocation().isEmpty()) {
        QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                       + QStringLiteral("/WindowsAppSDK");
        QDir().mkpath(path);
        downloadLocation.setValue(path);
    }

    if (windowsAppSdkLocation().isEmpty()) {
        windowsAppSdkLocation.setValue(FilePath::fromUserInput(
            Environment::systemEnvironment().value(Constants::WINDOWS_WINAPPSDK_ROOT_ENV_KEY)));
    }
}

static bool isHttpRedirect(QNetworkReply *reply)
{
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 305
           || statusCode == 307 || statusCode == 308;
}

// TODO: Make it a separate async task in a chain?
static std::optional<QString> saveToDisk(const FilePath &filename, QIODevice *data)
{
    const Result<qint64> result = filename.writeFileContents(data->readAll());
    if (!result) {
        return Tr::tr("Could not open \"%1\" for writing: %2.")
        .arg(filename.toUserOutput(), result.error());
    }

    return {};
}

class WindowsSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    WindowsSettingsWidget();

private:
    void showEvent(QShowEvent *event) override;

    GroupItem downloadNugetRecipe();
    void downloadNuget();
    void downloadWindowsAppSdk();

    void updateUI();

    void validateDownloadPath();
    void validateNuget();
    void validateWindowsAppSdk();

    bool m_isInitialReloadDone = false;

    SummaryWidget *m_winAppSdkSummary = nullptr;
    PathChooser *m_downloadPathChooser;
    PathChooser *m_nugetPathChooser;
    PathChooser *m_winAppSdkPathChooser;
    QNetworkAccessManager manager;
    Tasking::TaskTreeRunner m_nugetDownloader;
};

enum WindowsAppSdkValidation {
    DownloadPathExistsRow,
    NugetPathExistsRow,
    WindowsAppSdkPathExists
};

WindowsSettingsWidget::WindowsSettingsWidget()
{
    setWindowTitle(Tr::tr("Windows Configuration"));

    auto winAppSdkDetailsWidget = new DetailsWidget;

    m_downloadPathChooser = new PathChooser;
    m_downloadPathChooser->setToolTip(Tr::tr("Select the download path of NuGet and Windows App SDK."));
    m_downloadPathChooser->setPromptDialogTitle(Tr::tr("Select Download Path"));
    m_downloadPathChooser->setExpectedKind(PathChooser::ExistingDirectory);
    m_downloadPathChooser->setFilePath(windowsAppSdkSettings().downloadLocation());

    m_nugetPathChooser = new PathChooser;
    m_nugetPathChooser->setToolTip(Tr::tr("Select the path of NuGet."));
    m_nugetPathChooser->setPromptDialogTitle(Tr::tr("Select nuget.exe File"));
    m_nugetPathChooser->setExpectedKind(PathChooser::Any);
    m_nugetPathChooser->setFilePath(windowsAppSdkSettings().nugetLocation());

    auto downloadNuget = new QPushButton(Tr::tr("Download NuGet"));
    downloadNuget->setToolTip(
        Tr::tr("Automatically download NuGet.\n\n"
               "NuGet is needed for downloading Windows App SDK."));

    m_winAppSdkPathChooser = new PathChooser;
    m_winAppSdkPathChooser->setToolTip(Tr::tr("Select the path of the Windows App SDK."));

    auto downloadWindowsAppSdk = new QPushButton(Tr::tr("Download Windows App SDK"));
    downloadWindowsAppSdk->setToolTip(
        Tr::tr("Automatically download Windows App SDK with NuGet.\n\n"
               "If the automatic download fails, Qt Creator proposes to open the download URL\n"
               "in the system browser for manual download."));

    const QMap<int, QString> winAppSdkValidationPoints = {
        { DownloadPathExistsRow, Tr::tr("Download path exists.") },
        { NugetPathExistsRow, Tr::tr("NuGet path exists.") },
        { WindowsAppSdkPathExists, Tr::tr("Windows App SDK path exists.") }
    };
    m_winAppSdkSummary = new SummaryWidget(
        winAppSdkValidationPoints,
        Tr::tr("Windows App SDK settings are OK."),
        Tr::tr("Windows App SDK settings have errors."),
        winAppSdkDetailsWidget);

    m_winAppSdkPathChooser->setPromptDialogTitle(Tr::tr("Select Windows App SDK Path"));
    WindowsAppSdkSettings &settings = windowsAppSdkSettings();
    if (settings.windowsAppSdkLocation().isEmpty())
        settings.windowsAppSdkLocation.setValue(settings.downloadLocation());
    m_winAppSdkPathChooser->setFilePath(settings.windowsAppSdkLocation());

    using namespace Layouting;

    Column {
        Layouting::Group {
            title(Tr::tr("Download Path")),
            Grid {
                Tr::tr("Download location:"),
                m_downloadPathChooser,
                br,

                Span(4, winAppSdkDetailsWidget)
            }
        },
        Layouting::Group {
            title(Tr::tr("NuGet")),
            Grid {
                Tr::tr("NuGet location:"),
                m_nugetPathChooser,
                downloadNuget,
                br,

                Span(4, winAppSdkDetailsWidget)
            }
        },
        Layouting::Group {
            title(Tr::tr("Windows App SDK Settings")),
            Grid {
                Tr::tr("Windows App SDK location:"),
                m_winAppSdkPathChooser,
                downloadWindowsAppSdk,
                br,

                Span(4, winAppSdkDetailsWidget)
            }
        },
        st
    }.attachTo(this);

    connect(m_downloadPathChooser, &PathChooser::rawPathChanged,
            this, &WindowsSettingsWidget::validateDownloadPath);
    connect(m_winAppSdkPathChooser, &PathChooser::rawPathChanged,
            this, &WindowsSettingsWidget::validateWindowsAppSdk);
    connect(m_nugetPathChooser, &PathChooser::rawPathChanged,
            this, &WindowsSettingsWidget::validateNuget);
    connect(downloadNuget, &QAbstractButton::clicked,
            this, &WindowsSettingsWidget::downloadNuget);
    connect(downloadWindowsAppSdk, &QAbstractButton::clicked,
            this, &WindowsSettingsWidget::downloadWindowsAppSdk);
    connect(&m_nugetDownloader, &Tasking::TaskTreeRunner::done, this,
        [this](Tasking::DoneWith result) {
            if (result != Tasking::DoneWith::Success)
                return;

            validateNuget();
            m_nugetPathChooser->triggerChanged(); // After cloning, the path exists
            updateUI();
            apply();
        });

    setOnApply([] { windowsAppSdkSettings().writeSettings(); });
}

void WindowsSettingsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!m_isInitialReloadDone) {
        validateDownloadPath();
        validateNuget();
        validateWindowsAppSdk();
        m_isInitialReloadDone = true;
    }
}

void WindowsSettingsWidget::validateDownloadPath()
{
    windowsAppSdkSettings().downloadLocation.setValue(m_downloadPathChooser->filePath());

    m_winAppSdkSummary->setPointValid(
        DownloadPathExistsRow, m_downloadPathChooser->filePath().exists());

    updateUI();
}

void WindowsSettingsWidget::validateNuget()
{
    windowsAppSdkSettings().nugetLocation.setValue(m_nugetPathChooser->filePath());

    m_winAppSdkSummary->setPointValid(NugetPathExistsRow, m_nugetPathChooser->filePath().exists());

    updateUI();
}

void WindowsSettingsWidget::validateWindowsAppSdk()
{
    windowsAppSdkSettings().windowsAppSdkLocation.setValue(m_winAppSdkPathChooser->filePath());

    QStringList filters;
    filters << "Microsoft.WindowsAppSDK.*.nupkg";
    QDir dir(windowsAppSdkSettings().windowsAppSdkLocation().path());
    auto results = dir.entryList(filters);
    m_winAppSdkSummary->setPointValid(WindowsAppSdkPathExists, results.count() > 0);

    updateUI();
}

GroupItem WindowsSettingsWidget::downloadNugetRecipe()
{
    const FilePath downloadPath = m_downloadPathChooser->filePath();
    const QString nugetUrl("https://dist.nuget.org/win-x86-commandline/latest/nuget.exe");

    const auto failDialog = [=](const QString &msgSuffix = {}) {
        QStringList sl;
        sl << Tr::tr("NuGet download failed.");
        if (!msgSuffix.isEmpty())
            sl << msgSuffix;
        sl << Tr::tr("Open NuGet URL for manual download?");
        QMessageBox msgBox;
        msgBox.setText(sl.join(" "));
        msgBox.addButton(Tr::tr("Cancel"), QMessageBox::RejectRole);
        QAbstractButton *openButton = msgBox.addButton(Tr::tr("Open Download URL"),
                                                       QMessageBox::ActionRole);
        msgBox.exec();

        if (msgBox.clickedButton() == openButton)
            QDesktopServices::openUrl(QUrl::fromUserInput("https://www.nuget.org/downloads"));
        openButton->deleteLater();
    };

    struct StorageStruct
    {
        StorageStruct() {
            progressDialog.reset(createProgressDialog(100, Tr::tr("Downloading"),
                                                      Tr::tr("Downloading NuGet...")));
        }
        std::unique_ptr<QProgressDialog> progressDialog;
        std::optional<FilePath> fileName;
    };

    Storage<StorageStruct> storage;

    const auto onSetup = [downloadPath, failDialog] {
        if (downloadPath.isEmpty()) {
            failDialog(Tr::tr("The SDK Tools download URL is empty."));
            return SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };

    const auto onQuerySetup = [storage, nugetUrl, failDialog](NetworkQuery &query) {
        query.setRequest(QNetworkRequest(QUrl(nugetUrl)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        QProgressDialog *progressDialog = storage->progressDialog.get();
        QObject::connect(&query, &NetworkQuery::downloadProgress,
                         progressDialog, [progressDialog](qint64 received, qint64 max) {
                             progressDialog->setRange(0, max);
                             progressDialog->setValue(received);
                         });
#if QT_CONFIG(ssl)
        QObject::connect(&query, &NetworkQuery::sslErrors,
                         &query, [queryPtr = &query, failDialog](const QList<QSslError> &sslErrs) {
                             for (const QSslError &error : sslErrs)
                                 qCDebug(windowssettingswidget, "SSL error: %s\n",
                                         qPrintable(error.errorString()));
                             failDialog(Tr::tr("Encountered SSL errors, download is aborted."));
                             queryPtr->reply()->abort();
                         });
#endif
    };
    const auto onQueryDone = [this,
                              storage,
                              failDialog,
                              downloadPath](const NetworkQuery &query, DoneWith result) {
        if (result == DoneWith::Cancel)
            return;

        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return);
        const QUrl url = reply->url();
        if (result != DoneWith::Success) {
            failDialog(Tr::tr("Downloading NuGet from URL %1 has failed: %2.")
                         .arg(url.toString(), reply->errorString()));
            return;
        }
        if (isHttpRedirect(reply)) {
            failDialog(Tr::tr("Download from %1 was redirected.").arg(url.toString()));
            return;
        }
        const QString path = url.path();
        QString basename = QFileInfo(path).fileName();
        const FilePath fileName = downloadPath / basename;
        const std::optional<QString> saveResult = saveToDisk(fileName, reply);
        if (saveResult) {
            failDialog(*saveResult);
            return;
        }
        storage->fileName = fileName;
        m_nugetPathChooser->setFilePath(fileName);
    };
    const auto onCancelSetup = [storage] { return std::make_pair(storage->progressDialog.get(),
                                                                 &QProgressDialog::canceled); };

    return Group {
        storage,
        Group {
            onGroupSetup(onSetup),
            NetworkQueryTask(onQuerySetup, onQueryDone),
        }.withCancel(onCancelSetup)
    };
}

void WindowsSettingsWidget::downloadNuget()
{
    const FilePath downloadPath = m_downloadPathChooser->filePath();
    const FilePath nugetPath = m_nugetPathChooser->filePath();
    const QString nugetDownloadingTitle(Tr::tr("Downloading"));

    if (nugetPath.exists() && nugetPath.isFile() && !nugetPath.isEmpty()) {
        QMessageBox::information(
            this,
            nugetDownloadingTitle,
            Tr::tr(
                "The selected download path (%1) for NuGet already exists.\n"
                "Select a different path.")
                .arg(nugetPath.toUserOutput()));
        return;
    }

    if (!m_winAppSdkSummary->rowsOk({DownloadPathExistsRow})) {
        QMessageBox::information(this, nugetDownloadingTitle,
                                 Tr::tr("Download path is not configured."));
        return;
    }

    m_nugetDownloader.start({downloadNugetRecipe()});
}

void WindowsSettingsWidget::downloadWindowsAppSdk()
{
    const FilePath downloadPath = m_downloadPathChooser->filePath();
    const FilePath winAppSdkPath = m_winAppSdkPathChooser->filePath();
    const FilePath nugetPath = m_nugetPathChooser->filePath();
    const QString winAppSdkDownloadTitle(Tr::tr("Downloading Windows App SDK"));
    const QString winAppSdkDownloadUrl = "https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/downloads";

    if (m_winAppSdkSummary->rowsOk({WindowsAppSdkPathExists})) {
        QMessageBox::information(this, winAppSdkDownloadTitle,
            Tr::tr("Windows App SDK is already configured."));
        return;
    }

    if (!m_winAppSdkSummary->rowsOk({DownloadPathExistsRow})) {
        QMessageBox::information(this, winAppSdkDownloadTitle,
                                 Tr::tr("Download path is not configured."));
        return;
    }

    QProgressDialog *winAppSdkProgressDialog
        = new QProgressDialog(Tr::tr("Downloading Windows App SDK..."),
                              Tr::tr("Cancel"), 0, 0);
    winAppSdkProgressDialog->setWindowModality(Qt::ApplicationModal);
    winAppSdkProgressDialog->setWindowTitle(winAppSdkDownloadTitle);
    winAppSdkProgressDialog->setFixedSize(winAppSdkProgressDialog->sizeHint());

    const QString winAppSdkLibraryName("Microsoft.WindowsAppSDK");
    Process *nugetDownloader = new Process(this);
    const CommandLine gitCloneCommand(nugetPath, {"install",
                                                  winAppSdkLibraryName,
                                                  "-OutputDirectory",
                                                  downloadPath.path()});
    nugetDownloader->setCommand(gitCloneCommand);

    qCDebug(windowssettingswidget) << "Downloading Windows App SDK: "
                                   << gitCloneCommand.toUserOutput();

    connect(winAppSdkProgressDialog,
            &QProgressDialog::canceled, nugetDownloader, &QObject::deleteLater);

    const auto failDialog = [=](const QString &msgSuffix = {}) {
        QStringList sl;
        sl << Tr::tr("Windows App SDK download failed.");
        if (!msgSuffix.isEmpty())
            sl << msgSuffix;
        sl << Tr::tr("Open Windows App SDK URL for manual download?");
        QMessageBox msgBox;
        msgBox.setText(sl.join(" "));
        msgBox.addButton(Tr::tr("Cancel"), QMessageBox::RejectRole);
        QAbstractButton *openButton = msgBox.addButton(Tr::tr("Open Download URL"),
                                                       QMessageBox::ActionRole);
        msgBox.exec();

        if (msgBox.clickedButton() == openButton)
            QDesktopServices::openUrl(QUrl::fromUserInput(winAppSdkDownloadUrl));
        openButton->deleteLater();
    };

    connect(nugetDownloader,
            &Process::done,
            this,
            [this, winAppSdkProgressDialog, nugetDownloader, failDialog, downloadPath] {
        winAppSdkProgressDialog->close();
        if (nugetDownloader->error() != QProcess::UnknownError) {
            if (nugetDownloader->error() == QProcess::FailedToStart) {
                failDialog();
                return;
            } else {
                failDialog();
            }
        }
        QStringList filters;
        filters << "Microsoft.WindowsAppSDK.*";
        QDir dir(downloadPath.path());
        auto results = dir.entryList(filters);
        if (results.count() > 0) {
            dir.cd(results[0]);
            m_winAppSdkPathChooser->setFilePath(FilePath::fromString(dir.path()));
        }
        validateWindowsAppSdk();
        m_winAppSdkPathChooser->triggerChanged(); // After cloning, the path exists
        nugetDownloader->deleteLater();

        if (!winAppSdkProgressDialog->wasCanceled()
                || nugetDownloader->result() == ProcessResult::FinishedWithError) {
            failDialog();
        }
        updateUI();
        apply();
    });

    winAppSdkProgressDialog->show();
    nugetDownloader->start();
}

void WindowsSettingsWidget::updateUI()
{
    const bool allOk = m_winAppSdkSummary->allRowsOk();
    m_winAppSdkSummary->setSetupOk(allOk);
}

// WindowsSettingsPage

class WindowsSettingsPage final : public Core::IOptionsPage
{
public:
    WindowsSettingsPage()
    {
        setId(Constants::WINDOWS_SETTINGS_ID);
        setDisplayName(Tr::tr("Windows App SDK"));
        setCategory(Constants::SDK_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new WindowsSettingsWidget; });
    }
};

void setupWindowsAppSdkSettings()
{
    if (!HostOsInfo::isWindowsHost())
        return;

    static WindowsSettingsPage theWindowsSettingsPage;

    (void) windowsAppSdkSettings();
}

} // namespace ProjectExplorer::Internal
