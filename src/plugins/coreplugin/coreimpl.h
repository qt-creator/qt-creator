/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef COREIMPL_H
#define COREIMPL_H

#include "icore.h"

namespace Core {
namespace Internal {
class MainWindow;

class CoreImpl : public ICore
{
    Q_OBJECT

public:
    CoreImpl(MainWindow *mainwindow);
    ~CoreImpl();

    void showNewItemDialog(const QString &title,
                                  const QList<IWizard *> &wizards,
                                  const QString &defaultLocation = QString());
    bool showOptionsDialog(const QString &group = QString(),
                           const QString &page = QString(),
                           QWidget *parent = 0);
    bool showWarningWithOptions(const QString &title, const QString &text,
                                const QString &details = QString(),
                                const QString &settingsCategory = QString(),
                                const QString &settingsId = QString(),
                                QWidget *parent = 0);

    ActionManager *actionManager() const;
    FileManager *fileManager() const ;
    UniqueIDManager *uniqueIDManager() const;
    MessageManager *messageManager() const;
    EditorManager *editorManager() const;
    ProgressManager *progressManager() const;
    ScriptManager *scriptManager() const;
    VariableManager *variableManager() const;
    VcsManager *vcsManager() const;
    ModeManager *modeManager() const;
    MimeDatabase *mimeDatabase() const;
    HelpManager *helpManager() const;

    QSettings *settings(QSettings::Scope scope = QSettings::UserScope) const;
    SettingsDatabase *settingsDatabase() const;
    QPrinter *printer() const;
    QString userInterfaceLanguage() const;

    QString resourcePath() const;
    QString userResourcePath() const;

    IContext *currentContextObject() const;

    QMainWindow *mainWindow() const;
    QStatusBar *statusBar() const;

    // Adds and removes additional active contexts, these contexts are appended
    // to the currently active contexts.
    void updateAdditionalContexts(const Context &remove, const Context &add);
    bool hasContext(int context) const;
    void addContextObject(IContext *context);
    void removeContextObject(IContext *context);

    void openFiles(const QStringList &fileNames, ICore::OpenFilesFlags flags);

    void emitNewItemsDialogRequested();

private:
    MainWindow *m_mainwindow;
    friend class MainWindow;
};

} // namespace Internal
} // namespace Core

#endif // COREIMPL_H
