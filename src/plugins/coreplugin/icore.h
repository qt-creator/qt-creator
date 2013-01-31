/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ICORE_H
#define ICORE_H

#include "core_global.h"
#include "id.h"

#include <QObject>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QPrinter;
class QStatusBar;
class QWidget;
template <class T> class QList;
QT_END_NAMESPACE


namespace Core {
class IWizard;
class ActionManager;
class Context;
class EditorManager;
class DocumentManager;
class HelpManager;
class IContext;
class MessageManager;
class MimeDatabase;
class ModeManager;
class ProgressManager;
class ScriptManager;
class SettingsDatabase;
class VariableManager;
class VcsManager;

namespace Internal { class MainWindow; }

class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

    friend class Internal::MainWindow;
    explicit ICore(Internal::MainWindow *mw);
    ~ICore();

public:
    // This should only be used to acccess the signals, so it could
    // theoretically return an QObject *. For source compatibility
    // it returns a ICore.
    static ICore *instance();

    static void showNewItemDialog(const QString &title,
                                  const QList<IWizard *> &wizards,
                                  const QString &defaultLocation = QString(),
                                  const QVariantMap &extraVariables = QVariantMap());

    static bool showOptionsDialog(Id group, Id page, QWidget *parent = 0);

    static bool showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       Id settingsCategory = Id(),
                                       Id settingsId = Id(),
                                       QWidget *parent = 0);

    static QT_DEPRECATED ActionManager *actionManager(); // Use Actionmanager::... directly.
    static QT_DEPRECATED DocumentManager *documentManager(); // Use DocumentManager::... directly.
    static MessageManager *messageManager();
    static EditorManager *editorManager();
    static ProgressManager *progressManager();
    static ScriptManager *scriptManager();
    static VariableManager *variableManager();
    static VcsManager *vcsManager();
    static QT_DEPRECATED ModeManager *modeManager(); // Use ModeManager::... directly.
    static MimeDatabase *mimeDatabase();
    static HelpManager *helpManager();

    static QSettings *settings(QSettings::Scope scope = QSettings::UserScope);
    static SettingsDatabase *settingsDatabase();
    static QPrinter *printer();
    static QString userInterfaceLanguage();

    static QString resourcePath();
    static QString userResourcePath();

    static QWidget *mainWindow();
    static QStatusBar *statusBar();

    static IContext *currentContextObject();
    // Adds and removes additional active contexts, these contexts are appended
    // to the currently active contexts.
    static void updateAdditionalContexts(const Context &remove, const Context &add);
    static void addContextObject(IContext *context);
    static void removeContextObject(IContext *context);

    enum OpenFilesFlags {
        None = 0,
        SwitchMode = 1,
        CanContainLineNumbers = 2,
         /// Stop loading once the first file fails to load
        StopOnLoadFail = 4
    };
    static void openFiles(const QStringList &fileNames, OpenFilesFlags flags = None);

    static void emitNewItemsDialogRequested();

    static void saveSettings();

signals:
    void coreAboutToOpen();
    void coreOpened();
    void newItemsDialogRequested();
    void saveSettingsRequested();
    void optionsDialogRequested();
    void coreAboutToClose();
    void contextAboutToChange(Core::IContext *context);
    void contextChanged(Core::IContext *context, const Core::Context &additionalContexts);
};

} // namespace Core

#endif // ICORE_H
