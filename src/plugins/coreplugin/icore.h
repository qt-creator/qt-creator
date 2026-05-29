// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "icontext.h"

#include <extensionsystem/pluginspec.h>
#include <utils/appmainwindow.h>
#include <utils/filepath.h>
#include <utils/qtcsettings.h>

#include <QList>
#include <QRect>

#include <functional>

QT_BEGIN_NAMESPACE
class QColor;
class QMainWindow;
class QPrinter;
class QStatusBar;
class QWidget;
QT_END_NAMESPACE

namespace Utils { class InfoBar; }

namespace Core {

class Context;
class IDocument;
class IWizardFactory;

class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

public:
    ICore();
    ~ICore() override;

    enum class ContextPriority {
        High,
        Low
    };

    static ICore *instance();

    static bool isNewItemDialogRunning();
    static QWidget *newItemDialog();
    static void showNewItemDialog(const QString &title,
                                  const QList<IWizardFactory *> &factories,
                                  const Utils::FilePath &defaultLocation = {},
                                  const QVariantMap &extraVariables = {});

    static void showSettings(const Utils::Id page);
    static void showSettings(const Utils::Id page, Utils::Id item);

    static QString msgShowSettings();
    static QString msgShowSettingsToolTip();

    static void showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       Utils::Id settingsId = {});

    static Utils::QtcSettings *settings(QSettings::Scope scope = QSettings::UserScope);
    static QPrinter *printer();
    static QString userInterfaceLanguage();

    static Utils::FilePath resourcePath(const QString &rel = {});
    static Utils::FilePath userResourcePath(const QString &rel = {});
    static Utils::FilePath cacheResourcePath(const QString &rel = {});
    static Utils::FilePath installerResourcePath(const QString &rel = {});
    static Utils::FilePath libexecPath(const QString &rel = {});
    static Utils::FilePath crashReportsPath();

    static QString versionString();

    static QMainWindow *mainWindow();
    static QWidget *dialogParent();
    static Utils::InfoBar *infoBar();
    static Utils::InfoBar *popupInfoBar();

    static QString msgRestartNow();
    static bool askForRestart(const QString &text, const QString &altButtonText = {});

    static void raiseWindow(QWidget *widget);
    static void raiseMainWindow();

    static QList<IContext *> currentContextObjects();
    static QWidget *currentContextWidget();
    static QList<IContext *> contextObjects(QWidget *widget);
    static void updateAdditionalContexts(const Context &remove, const Context &add,
                                         ContextPriority priority = ContextPriority::Low);
    static void addAdditionalContext(const Context &context,
                                     ContextPriority priority = ContextPriority::Low);
    static void removeAdditionalContext(const Context &context);
    static void addContextObject(IContext *context);
    static void removeContextObject(IContext *context);

    static void registerWindow(QWidget *window,
                               const Context &context,
                               const Context &actionContext = {});
    static void restartTrimmer();

    enum OpenFilesFlag {
        None = 0,
        SwitchMode = 1,
        CanContainLineAndColumnNumbers = 2,
         /// Stop loading once the first file fails to load
        StopOnLoadFail = 4,
        SwitchSplitIfAlreadyVisible = 8
    };
    Q_DECLARE_FLAGS(OpenFilesFlags, OpenFilesFlag)

    static void addPreCloseListener(const std::function<bool()> &listener);

    static void restart();

    static bool enablePlugins(const QSet<ExtensionSystem::PluginSpec *> &plugins);

    enum SaveSettingsReason {
        SettingsDialogDone,
        ModeChanged,
        MainWindowClosing,
    };

    // If there are dirty settings, ask the user if they want to apply them.
    // The callback is called if the user decides to apply the changed settings.
    // If the user cancels the dialog, the callback is never called.
    // If no settings are dirty, the callback is called immediately.
    static void askToApplySettings(const std::function<void()> &callback);

public slots:
    static void openFileWith();
    static void exit();

signals:
    void coreAboutToOpen();
    void coreOpened();
    void newItemDialogStateChanged();
    void wizardFinished(const Utils::Id &id, bool accepted);
    void askToApplySettingsRequested(const std::function<void()> &callback);
    void saveSettingsRequested(SaveSettingsReason reason);
    void coreAboutToClose();
    void contextAboutToChange(const QList<Core::IContext *> &context);
    void contextChanged(const Core::Context &context);
    void systemEnvironmentChanged();

public:
    /* internal use */
    static void setRelativePathToProjectFunction(const std::function<Utils::FilePath(const Utils::FilePath &)> &func);
    static Utils::FilePath pathRelativeToActiveProject(const Utils::FilePath &path);
    static QStringList additionalAboutInformation();
    static void clearAboutInformation();
    static void setPrependAboutInformation(const QString &line);
    static void appendAboutInformation(const QString &line);
    static QString aboutInformationCompact();
    static QString aboutInformationHtml();
    static QString systemInformation();
    static void setupScreenShooter(const QString &name, QWidget *w, const QRect &rc = QRect());
    static Utils::Result<Utils::FilePath> clangExecutable(const Utils::FilePath &clangBinDirectory);
    static Utils::Result<Utils::FilePath> clangdExecutable(const Utils::FilePath &clangBinDirectory);
    static Utils::Result<Utils::FilePath> clangTidyExecutable(const Utils::FilePath &clangBinDirectory);
    static Utils::Result<Utils::FilePath> clazyStandaloneExecutable(const Utils::FilePath &clangBinDirectory);
    static Utils::FilePath clangIncludeDirectory(const QString &clangVersion,
                                                 const Utils::FilePath &clangFallbackIncludeDir);
    static Utils::Result<Utils::FilePath> lldbExecutable(const Utils::FilePath &lldbBinDirectory);
    static QStatusBar *statusBar();

    static void saveSettings(SaveSettingsReason reason);
    static void updateNewItemDialogState();

    static void setOverrideColor(const QColor &color);

    static void init();
    static void extensionsInitialized();
    static void aboutToShutdown();
    static void saveSettings();

    static IDocument *openFiles(
        const Utils::FilePaths &filePaths,
        OpenFilesFlags flags = None,
        const Utils::FilePath &workingDirectory = {},
        bool openProjects = true);
};

} // namespace Core
