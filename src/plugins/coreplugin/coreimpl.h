/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
    EditorManager *editorManager() const;
    ProgressManager *progressManager() const;
    ScriptManager *scriptManager() const;
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
};

} // namespace Internal
} // namespace Core

#endif // COREIMPL_H
