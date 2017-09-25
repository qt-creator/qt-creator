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
#include "id.h"

#include <QObject>
#include <QSettings>

#include <functional>

QT_BEGIN_NAMESPACE
class QPrinter;
class QStatusBar;
class QWidget;
template <typename T> class QList;
QT_END_NAMESPACE

namespace Core {
class IWizardFactory;
class Context;
class IContext;
class SettingsDatabase;

namespace Internal { class MainWindow; }

class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

    friend class Internal::MainWindow;
    friend class IWizardFactory;

    explicit ICore(Internal::MainWindow *mw);
    ~ICore();

public:
    enum class ContextPriority {
        High,
        Low
    };

    // This should only be used to acccess the signals, so it could
    // theoretically return an QObject *. For source compatibility
    // it returns a ICore.
    static ICore *instance();

    static bool isNewItemDialogRunning();
    static QWidget *newItemDialog();
    static void showNewItemDialog(const QString &title,
                                  const QList<IWizardFactory *> &factories,
                                  const QString &defaultLocation = QString(),
                                  const QVariantMap &extraVariables = QVariantMap());

    static bool showOptionsDialog(Id page, QWidget *parent = 0);
    static QString msgShowOptionsDialog();
    static QString msgShowOptionsDialogToolTip();

    static bool showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       Id settingsId = Id(),
                                       QWidget *parent = 0);

    static QSettings *settings(QSettings::Scope scope = QSettings::UserScope);
    static SettingsDatabase *settingsDatabase();
    static QPrinter *printer();
    static QString userInterfaceLanguage();

    static QString resourcePath();
    static QString userResourcePath();
    static QString documentationPath();
    static QString libexecPath();

    static QString versionString();
    static QString buildCompatibilityString();

    static QWidget *mainWindow();
    static QWidget *dialogParent();
    static QStatusBar *statusBar();
    /* Raises and activates the window for the widget. This contains workarounds for X11. */
    static void raiseWindow(QWidget *widget);

    static IContext *currentContextObject();
    // Adds and removes additional active contexts, these contexts are appended
    // to the currently active contexts.
    static void updateAdditionalContexts(const Context &remove, const Context &add,
                                         ContextPriority priority = ContextPriority::Low);
    static void addAdditionalContext(const Context &context,
                                     ContextPriority priority = ContextPriority::Low);
    static void removeAdditionalContext(const Context &context);
    static void addContextObject(IContext *context);
    static void removeContextObject(IContext *context);

    // manages the minimize, zoom and fullscreen actions for the window
    static void registerWindow(QWidget *window, const Context &context);

    enum OpenFilesFlags {
        None = 0,
        SwitchMode = 1,
        CanContainLineAndColumnNumbers = 2,
         /// Stop loading once the first file fails to load
        StopOnLoadFail = 4
    };
    static void openFiles(const QStringList &fileNames, OpenFilesFlags flags = None);

    static void addPreCloseListener(const std::function<bool()> &listener);

    static QString systemInformation();

public slots:
    static void saveSettings();

signals:
    void coreAboutToOpen();
    void coreOpened();
    void newItemDialogStateChanged();
    void saveSettingsRequested();
    void optionsDialogRequested();
    void coreAboutToClose();
    void contextAboutToChange(const QList<Core::IContext *> &context);
    void contextChanged(const Core::Context &context);

public:
    /* internal use */
    static QStringList additionalAboutInformation();
    static void appendAboutInformation(const QString &line);

private:
    static void updateNewItemDialogState();
};

} // namespace Core
