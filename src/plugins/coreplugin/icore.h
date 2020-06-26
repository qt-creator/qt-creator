/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "core_global.h"
#include "icontext.h"

#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QRect>
#include <QSettings>

#include <functional>

QT_BEGIN_NAMESPACE
class QPrinter;
class QStatusBar;
class QWidget;
QT_END_NAMESPACE

namespace Utils {
class InfoBar;
}

namespace Core {
class Context;
class IWizardFactory;
class SettingsDatabase;

namespace Internal { class MainWindow; }

class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

    friend class Internal::MainWindow;
    friend class IWizardFactory;

    explicit ICore(Internal::MainWindow *mw);
    ~ICore() override;

public:
    enum class ContextPriority {
        High,
        Low
    };

    static ICore *instance();

    static bool isNewItemDialogRunning();
    static QWidget *newItemDialog();
    static void showNewItemDialog(const QString &title,
                                  const QList<IWizardFactory *> &factories,
                                  const QString &defaultLocation = QString(),
                                  const QVariantMap &extraVariables = QVariantMap());

    static bool showOptionsDialog(const Utils::Id page, QWidget *parent = nullptr);
    static QString msgShowOptionsDialog();
    static QString msgShowOptionsDialogToolTip();

    static bool showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       Utils::Id settingsId = {},
                                       QWidget *parent = nullptr);

    static QSettings *settings(QSettings::Scope scope = QSettings::UserScope);
    static SettingsDatabase *settingsDatabase();
    static QPrinter *printer();
    static QString userInterfaceLanguage();

    static QString resourcePath();
    static QString userResourcePath();
    static QString cacheResourcePath();
    static QString installerResourcePath();
    static QString libexecPath();

    static QString versionString();

    static QMainWindow *mainWindow();
    static QWidget *dialogParent();
    static Utils::InfoBar *infoBar();

    static void raiseWindow(QWidget *widget);

    static IContext *currentContextObject();
    static QWidget *currentContextWidget();
    static IContext *contextObject(QWidget *widget);
    static void updateAdditionalContexts(const Context &remove, const Context &add,
                                         ContextPriority priority = ContextPriority::Low);
    static void addAdditionalContext(const Context &context,
                                     ContextPriority priority = ContextPriority::Low);
    static void removeAdditionalContext(const Context &context);
    static void addContextObject(IContext *context);
    static void removeContextObject(IContext *context);

    static void registerWindow(QWidget *window, const Context &context);

    enum OpenFilesFlags {
        None = 0,
        SwitchMode = 1,
        CanContainLineAndColumnNumbers = 2,
         /// Stop loading once the first file fails to load
        StopOnLoadFail = 4,
        SwitchSplitIfAlreadyVisible = 8
    };
    static void openFiles(const QStringList &fileNames, OpenFilesFlags flags = None);

    static void addPreCloseListener(const std::function<bool()> &listener);

    static void restart();

    enum SaveSettingsReason {
        InitializationDone,
        SettingsDialogDone,
        ModeChanged,
        MainWindowClosing,
    };

signals:
    void coreAboutToOpen();
    void coreOpened();
    void newItemDialogStateChanged();
    void saveSettingsRequested(SaveSettingsReason reason);
    void coreAboutToClose();
    void contextAboutToChange(const QList<Core::IContext *> &context);
    void contextChanged(const Core::Context &context);

public:
    /* internal use */
    static QStringList additionalAboutInformation();
    static void appendAboutInformation(const QString &line);
    static QString systemInformation();
    static void setupScreenShooter(const QString &name, QWidget *w, const QRect &rc = QRect());
    static QString pluginPath();
    static QString userPluginPath();
    static QString clangExecutable(const QString &clangBinDirectory);
    static QString clangTidyExecutable(const QString &clangBinDirectory);
    static QString clazyStandaloneExecutable(const QString &clangBinDirectory);
    static QString clangIncludeDirectory(const QString &clangVersion,
                                         const QString &clangResourceDirectory);
    static QString buildCompatibilityString();
    static QStatusBar *statusBar();

    static void saveSettings(SaveSettingsReason reason);

private:
    static void updateNewItemDialogState();
};

} // namespace Core
