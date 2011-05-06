/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ICORE_H
#define ICORE_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QSettings>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QPrinter;
class QStatusBar;
template <class T> class QList;
QT_END_NAMESPACE


namespace Core {
class IWizard;
class ActionManager;
class Context;
class EditorManager;
class FileManager;
class HelpManager;
class IContext;
class MessageManager;
class MimeDatabase;
class ModeManager;
class ProgressManager;
class ScriptManager;
class SettingsDatabase;
class UniqueIDManager;
class VariableManager;
class VcsManager;

class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

public:
    ICore() {}
    virtual ~ICore() {}

    static ICore *instance();

    virtual void showNewItemDialog(const QString &title,
                                   const QList<IWizard *> &wizards,
                                   const QString &defaultLocation = QString()) = 0;

    virtual bool showOptionsDialog(const QString &group = QString(),
                                   const QString &page = QString(),
                                   QWidget *parent = 0) = 0;

    virtual bool showWarningWithOptions(const QString &title, const QString &text,
                                       const QString &details = QString(),
                                       const QString &settingsCategory = QString(),
                                       const QString &settingsId = QString(),
                                       QWidget *parent = 0) = 0;

    virtual ActionManager *actionManager() const = 0;
    virtual FileManager *fileManager() const = 0;
    virtual UniqueIDManager *uniqueIDManager() const = 0;
    virtual MessageManager *messageManager() const = 0;
    virtual EditorManager *editorManager() const = 0;
    virtual ProgressManager *progressManager() const = 0;
    virtual ScriptManager *scriptManager() const = 0;
    virtual VariableManager *variableManager() const = 0;
    virtual VcsManager *vcsManager() const = 0;
    virtual ModeManager *modeManager() const = 0;
    virtual MimeDatabase *mimeDatabase() const = 0;
    virtual HelpManager *helpManager() const = 0;

    virtual QSettings *settings(QSettings::Scope scope = QSettings::UserScope) const = 0;
    virtual SettingsDatabase *settingsDatabase() const = 0;
    virtual QPrinter *printer() const = 0;
    virtual QString userInterfaceLanguage() const = 0;

    virtual QString resourcePath() const = 0;
    virtual QString userResourcePath() const = 0;

    virtual QMainWindow *mainWindow() const = 0;
    virtual QStatusBar *statusBar() const = 0;

    virtual IContext *currentContextObject() const = 0;
    // Adds and removes additional active contexts, these contexts are appended
    // to the currently active contexts.
    virtual void updateAdditionalContexts(const Context &remove, const Context &add) = 0;
    virtual bool hasContext(int context) const = 0;
    virtual void addContextObject(IContext *context) = 0;
    virtual void removeContextObject(IContext *context) = 0;

    enum OpenFilesFlags {
        None = 0,
        SwitchMode = 1,
        CanContainLineNumbers = 2,
         /// Stop loading once the first file fails to load
        StopOnLoadFail = 4
    };
    virtual void openFiles(const QStringList &fileNames, OpenFilesFlags flags = None) = 0;

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
