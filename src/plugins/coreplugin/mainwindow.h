// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"
#include "icore.h"

#include <utils/appmainwindow.h>
#include <utils/dropsupport.h>

#include <QColor>
#include <QTimer>

#include <functional>
#include <unordered_map>

QT_BEGIN_NAMESPACE
class QPrinter;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class InfoBar;
}

namespace Core {

class EditorManager;
class ExternalToolManager;
class IDocument;
class JsExpander;
class MessageManager;
class ModeManager;
class ProgressManager;
class NavigationWidget;
enum class Side;
class RightPaneWidget;
class SettingsDatabase;
class VcsManager;

namespace Internal {

class FancyTabWidget;
class GeneralSettings;
class ProgressManagerPrivate;
class ShortcutSettings;
class ToolSettings;
class MimeTypeSettings;
class VersionDialog;
class WindowSupport;
class SystemEditor;
class SystemSettings;

class MainWindow : public Utils::AppMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

    void init();
    void extensionsInitialized();
    void aboutToShutdown();

    IContext *contextObject(QWidget *widget) const;
    void addContextObject(IContext *context);
    void removeContextObject(IContext *context);

    static IDocument *openFiles(const Utils::FilePaths &filePaths,
                                ICore::OpenFilesFlags flags,
                                const Utils::FilePath &workingDirectory = {});

    inline SettingsDatabase *settingsDatabase() const { return m_settingsDatabase; }
    virtual QPrinter *printer() const;
    IContext * currentContextObject() const;
    QStatusBar *statusBar() const;
    Utils::InfoBar *infoBar() const;

    void updateAdditionalContexts(const Context &remove, const Context &add,
                                  ICore::ContextPriority priority);

    bool askConfirmationBeforeExit() const;
    void setAskConfirmationBeforeExit(bool ask);

    void setOverrideColor(const QColor &color);

    QStringList additionalAboutInformation() const;
    void appendAboutInformation(const QString &line);

    void addPreCloseListener(const std::function<bool()> &listener);

    void saveSettings();

    void restart();

    void openFileFromDevice();

    void restartTrimmer();

public slots:
    static void openFileWith();
    void exit();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static void openFile();
    void aboutToShowRecentFiles();
    static void setFocusToEditor();
    void aboutQtCreator();
    void aboutPlugins();
    void changeLog();
    void contact();
    void updateFocusWidget(QWidget *old, QWidget *now);
    NavigationWidget *navigationWidget(Side side) const;
    void setSidebarVisible(bool visible, Side side);
    void destroyVersionDialog();
    void openDroppedFiles(const QList<Utils::DropSupport::FileSpec> &files);
    void restoreWindowState();

    void updateContextObject(const QList<IContext *> &context);
    void updateContext();

    void registerDefaultContainers();
    void registerDefaultActions();
    void registerModeSelectorStyleActions();

    void readSettings();
    void saveWindowSettings();

    void updateModeSelectorStyleMenu();

    ICore *m_coreImpl = nullptr;
    QTimer m_trimTimer;
    QStringList m_aboutInformation;
    Context m_highPrioAdditionalContexts;
    Context m_lowPrioAdditionalContexts;
    SettingsDatabase *m_settingsDatabase = nullptr;
    mutable QPrinter *m_printer = nullptr;
    WindowSupport *m_windowSupport = nullptr;
    EditorManager *m_editorManager = nullptr;
    ExternalToolManager *m_externalToolManager = nullptr;
    MessageManager *m_messageManager = nullptr;
    ProgressManagerPrivate *m_progressManager = nullptr;
    JsExpander *m_jsExpander = nullptr;
    VcsManager *m_vcsManager = nullptr;
    ModeManager *m_modeManager = nullptr;
    FancyTabWidget *m_modeStack = nullptr;
    NavigationWidget *m_leftNavigationWidget = nullptr;
    NavigationWidget *m_rightNavigationWidget = nullptr;
    RightPaneWidget *m_rightPaneWidget = nullptr;
    VersionDialog *m_versionDialog = nullptr;

    QList<IContext *> m_activeContext;

    std::unordered_map<QWidget *, IContext *> m_contextWidgets;

    GeneralSettings *m_generalSettings = nullptr;
    SystemSettings *m_systemSettings = nullptr;
    ShortcutSettings *m_shortcutSettings = nullptr;
    ToolSettings *m_toolSettings = nullptr;
    MimeTypeSettings *m_mimeTypeSettings = nullptr;
    SystemEditor *m_systemEditor = nullptr;

    // actions
    QAction *m_focusToEditor = nullptr;
    QAction *m_newAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_openWithAction = nullptr;
    QAction *m_openFromDeviceAction = nullptr;
    QAction *m_saveAllAction = nullptr;
    QAction *m_exitAction = nullptr;
    QAction *m_optionsAction = nullptr;
    QAction *m_loggerAction = nullptr;
    QAction *m_toggleLeftSideBarAction = nullptr;
    QAction *m_toggleRightSideBarAction = nullptr;
    QAction *m_cycleModeSelectorStyleAction = nullptr;
    QAction *m_setModeSelectorStyleIconsAndTextAction = nullptr;
    QAction *m_setModeSelectorStyleHiddenAction = nullptr;
    QAction *m_setModeSelectorStyleIconsOnlyAction = nullptr;
    QAction *m_themeAction = nullptr;

    QToolButton *m_toggleLeftSideBarButton = nullptr;
    QToolButton *m_toggleRightSideBarButton = nullptr;
    bool m_askConfirmationBeforeExit = false;
    QColor m_overrideColor;
    QList<std::function<bool()>> m_preCloseListeners;
};

} // namespace Internal
} // namespace Core
