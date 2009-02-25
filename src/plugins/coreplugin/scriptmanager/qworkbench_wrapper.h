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

#ifndef QWORKBENCH_WRAPPER_H
#define QWORKBENCH_WRAPPER_H

#include "metatypedeclarations.h" // required for property declarations

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtScript/QScriptable>
#include <QtScript/QScriptValue>

namespace Core {
namespace Internal {

// Script prototype for the core interface.

class CorePrototype : public QObject, public QScriptable
{
    Q_OBJECT

    Q_PROPERTY(Core::MessageManager* messageManager READ messageManager DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(Core::FileManager* fileManager READ fileManager DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(Core::EditorManager* editorManager READ editorManager DESIGNABLE false SCRIPTABLE true STORED false)

    Q_PROPERTY(QMainWindow* mainWindow READ mainWindow DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QSettings* settings READ settings DESIGNABLE false SCRIPTABLE true STORED false)

public:
    typedef Core::ICore ICore;

    CorePrototype(QObject *parent);

    Core::MessageManager *messageManager() const;
    Core::FileManager *fileManager() const;
    Core::EditorManager *editorManager() const;

    QMainWindow *mainWindow() const;
    QSettings *settings() const;

public slots:
    void addAdditionalContext(int context);
    void removeAdditionalContext(int context);
    QString toString() const;

private:
    ICore *callee() const;
};

// Script prototype for the message manager.

class MessageManagerPrototype : public QObject, public QScriptable
{
    Q_OBJECT

public:
    typedef Core::MessageManager MessageManager;

    MessageManagerPrototype(QObject *parent = 0);

public slots:
    void displayStatusBarMessage(const QString &text, int ms = 0);
    void printToOutputPane(const QString &text, bool bringToForeground = true);
    QString toString() const;
};

// Script prototype for the file manager interface.

class FileManagerPrototype : public QObject, public QScriptable
{
    Q_OBJECT

    Q_PROPERTY(QStringList recentFiles READ recentFiles DESIGNABLE false SCRIPTABLE true STORED false)

public:
    typedef Core::FileManager FileManager;

    FileManagerPrototype(QObject *parent = 0);
    QStringList recentFiles() const;

public slots:
    bool addFiles(const QList<Core::IFile *> &files);
    bool addFile(Core::IFile *file);
    bool removeFile(Core::IFile *file);

    QList<Core::IFile*> saveModifiedFilesSilently(const QList<Core::IFile*> &files);
    QString getSaveAsFileName(Core::IFile *file);

    bool isFileManaged(const QString &fileName) const;
    QList<Core::IFile *> managedFiles(const QString &fileName) const;

    void blockFileChange(Core::IFile *file);
    void unblockFileChange(Core::IFile *file);

    void addToRecentFiles(const QString &fileName);
    QString toString() const;

private:
    FileManager *callee() const;
};

// Script prototype for the file interface.

class FilePrototype : public QObject, public QScriptable
{
    Q_OBJECT

    Q_PROPERTY(QString fileName READ fileName DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QString defaultPath READ defaultPath DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QString suggestedFileName READ suggestedFileName DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(bool isModified READ isModified DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(bool isReadOnly READ isReadOnly DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(bool isSaveAsAllowed READ isSaveAsAllowed DESIGNABLE false SCRIPTABLE true STORED false)
public:
    typedef Core::IFile IFile;

    FilePrototype(QObject *parent = 0);

    QString fileName() const;
    QString defaultPath() const;
    QString suggestedFileName() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

public slots:
    QString toString() const;

private:
    IFile *callee() const;
};

// Script prototype for the editor manager interface.

class EditorManagerPrototype : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY(Core::IEditor* currentEditor READ currentEditor WRITE setCurrentEditor DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QList<Core::IEditor*> openedEditors READ openedEditors DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QList<Core::IEditor*> editorHistory  READ editorHistory DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QList<Core::EditorGroup *> editorGroups READ editorGroups DESIGNABLE false SCRIPTABLE true STORED false)
public:
    typedef  Core::EditorManager EditorManager;

    EditorManagerPrototype(QObject *parent = 0);

    Core::IEditor *currentEditor() const;
    void setCurrentEditor(Core::IEditor *editor);
    QList<Core::IEditor*> openedEditors() const;
    QList<Core::IEditor*> editorHistory() const;
    QList<Core::EditorGroup *> editorGroups() const;

public slots:
    QList<Core::IEditor*> editorsForFiles(QList<Core::IFile*> files) const;
    bool closeEditors(const QList<Core::IEditor*> editorsToClose, bool askAboutModifiedEditors);
    Core::IEditor *openEditor(const QString &fileName, const QString &editorKind);
    Core::IEditor *newFile(const QString &editorKind, QString titlePattern, const QString &contents);
    int makeEditorWritable(Core::IEditor *editor);

    QString toString() const;

private:
    EditorManager *callee() const;
};

// Script prototype for the editor interface.

class EditorPrototype :  public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QString kind READ kind DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(bool duplicateSupported READ duplicateSupported DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(Core::IFile* file READ file DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QToolBar* toolBar  READ toolBar DESIGNABLE false SCRIPTABLE true STORED false)

public:
    EditorPrototype(QObject *parent = 0);

    QString displayName() const;
    void setDisplayName(const QString &title);

    QString kind() const;
    bool duplicateSupported() const;

    Core::IFile *file() const;
    QToolBar* toolBar() const;

public slots:
    bool createNew(const QString &contents);
    bool open(const QString &fileName);
    Core::IEditor *duplicate(QWidget *parent);

    QString toString() const;

private:
    Core::IEditor *callee() const;
};

// Script prototype for the editor group interface with Script-managed life cycle.

class EditorGroupPrototype :  public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY(int editorCount READ editorCount DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(Core::IEditor* currentEditor READ currentEditor WRITE setCurrentEditor DESIGNABLE false SCRIPTABLE true STORED false)
    Q_PROPERTY(QList<Core::IEditor*> editors READ editors DESIGNABLE false SCRIPTABLE true STORED false)

public:
    EditorGroupPrototype(QObject *parent = 0);

    int editorCount() const;
    Core::IEditor *currentEditor() const;
    void setCurrentEditor(Core::IEditor *editor);
    QList<Core::IEditor*> editors() const;

public slots:
    void addEditor(Core::IEditor *editor);
    void insertEditor(int i, Core::IEditor *editor);
    void removeEditor(Core::IEditor *editor);

    void moveEditorsFromGroup(Core::EditorGroup *group);
    void moveEditorFromGroup(Core::EditorGroup *group, Core::IEditor *editor);

    QString toString() const;

private:
    Core::EditorGroup *callee() const;
};

} // namespace Internal
} // namespace Core

#endif // QWORKBENCH_WRAPPER_H
