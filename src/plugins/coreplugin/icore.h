/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef ICORE_H
#define ICORE_H

#include "core_global.h"
#include <extensionsystem/pluginmanager.h>
#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QSettings;
class QStatusBar;
class QFocusEvent;
class QMainWindow;
class QPrinter;
QT_END_NAMESPACE

namespace Core {

// forward declarations
class ActionManagerInterface;
class IFile;
class FileManager;
class MessageManager;
class IEditor;
class UniqueIDManager;
class ViewManagerInterface;
class EditorManager;
class ProgressManagerInterface;
class ScriptManagerInterface;
class VariableManager;
class IContext;
class VCSManager;
class ModeManager;
class IWizard;
class MimeDatabase;

class CORE_EXPORT ICore : public QObject
{
    Q_OBJECT

public:
    ICore() {}
    virtual ~ICore() {}

    virtual QStringList showNewItemDialog(const QString &title,
                                          const QList<IWizard *> &wizards,
                                          const QString &defaultLocation = QString()) = 0;

    virtual void showOptionsDialog(const QString &group = QString(),
                                   const QString &page = QString()) = 0;

    virtual ActionManagerInterface *actionManager() const = 0;
    virtual FileManager *fileManager() const = 0;
    virtual UniqueIDManager *uniqueIDManager() const = 0;
    virtual MessageManager *messageManager() const = 0;
    virtual ViewManagerInterface *viewManager() const = 0;
    virtual ExtensionSystem::PluginManager *pluginManager() const = 0;
    virtual EditorManager *editorManager() const = 0;
    virtual ProgressManagerInterface *progressManager() const = 0;
    virtual ScriptManagerInterface *scriptManager() const = 0;
    virtual VariableManager *variableManager() const = 0;
    virtual VCSManager *vcsManager() const = 0;
    virtual ModeManager *modeManager() const = 0;
    virtual MimeDatabase *mimeDatabase() const = 0;

    virtual QSettings *settings() const = 0;
    virtual QPrinter *printer() const = 0;

    virtual QString resourcePath() const = 0;
    virtual QString libraryPath() const = 0;

    virtual IContext *currentContextObject() const = 0;

    virtual QMainWindow *mainWindow() const = 0;
    virtual QStatusBar *statusBar() const = 0;

    // adds and removes additional active contexts, this context is appended to the
    // currently active contexts. call updateContext after changing
    virtual void addAdditionalContext(int context) = 0;
    virtual void removeAdditionalContext(int context) = 0;
    virtual bool hasContext(int context) const = 0;
    virtual void addContextObject(IContext *contex) = 0;
    virtual void removeContextObject(IContext *contex) = 0;

    virtual void updateContext() = 0;

    virtual void openFiles(const QStringList &fileNames) = 0;

signals:
    void coreOpened();
    void saveSettingsRequested();
    void settingsDialogRequested();
    void coreAboutToClose();
    void contextAboutToChange(Core::IContext *context);
    void contextChanged(Core::IContext *context);
};

} // namespace Core

#endif // ICORE_H
