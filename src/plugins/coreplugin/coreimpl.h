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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef COREIMPL_H
#define COREIMPL_H

#include "icore.h"
#include "mainwindow.h"

namespace Core {
namespace Internal {

class CoreImpl : public ICore
{
    Q_OBJECT

public:
    static CoreImpl *instance();

    CoreImpl(MainWindow *mainwindow);
    ~CoreImpl() {}

    QStringList showNewItemDialog(const QString &title,
                                  const QList<IWizard *> &wizards,
                                  const QString &defaultLocation = QString());
    void showOptionsDialog(const QString &group = QString(),
                                   const QString &page = QString());

    ActionManager *actionManager() const;
    FileManager *fileManager() const ;
    UniqueIDManager *uniqueIDManager() const;
    MessageManager *messageManager() const;
    ExtensionSystem::PluginManager *pluginManager() const;
    EditorManager *editorManager() const;
    ProgressManagerInterface *progressManager() const;
    ScriptManagerInterface *scriptManager() const;
    VariableManager *variableManager() const;
    VCSManager *vcsManager() const;
    ModeManager *modeManager() const;
    MimeDatabase *mimeDatabase() const;

    QSettings *settings() const;
    QPrinter *printer() const;

    QString resourcePath() const;

    IContext *currentContextObject() const;

    QMainWindow *mainWindow() const;

    // adds and removes additional active contexts, this context is appended to the
    // currently active contexts. call updateContext after changing
    void addAdditionalContext(int context);
    void removeAdditionalContext(int context);
    bool hasContext(int context) const;
    void addContextObject(IContext *contex);
    void removeContextObject(IContext *contex);

    void updateContext();

    void openFiles(const QStringList &fileNames);

private:
    MainWindow *m_mainwindow;
    friend class MainWindow;

    static CoreImpl *m_instance;
};

} // namespace Internal
} // namespace Core

#endif // COREIMPL_H
