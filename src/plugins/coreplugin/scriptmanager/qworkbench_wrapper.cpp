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

#include "qworkbench_wrapper.h"

#include <wrap_helpers.h>

#include <coreplugin/messagemanager.h>
#include <coreplugin/editormanager/editorgroup.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>

#include <QtScript/QScriptEngine>

namespace {
    enum { debugQWorkbenchWrappers = 0 };
}

namespace Core {
namespace Internal {

// -- CorePrototype
CorePrototype::CorePrototype(QObject *parent) :
    QObject(parent)
{
}

Core::MessageManager *CorePrototype::messageManager() const
{
    return callee()->messageManager();
}

Core::FileManager *CorePrototype::fileManager() const
{
     return callee()->fileManager();
}

Core::EditorManager *CorePrototype::editorManager() const
{
     return callee()->editorManager();
}

QMainWindow *CorePrototype::mainWindow() const
{
    return callee()->mainWindow();
}

QSettings *CorePrototype::settings() const
{
     return callee()->settings();
}

void CorePrototype::addAdditionalContext(int context)
{
    callee()->addAdditionalContext(context);
}

void CorePrototype::removeAdditionalContext(int context)
{
    callee()->removeAdditionalContext(context);
}

QString CorePrototype::toString() const
{
    return QLatin1String("Core");
}

CorePrototype::ICore *CorePrototype::callee() const
{
    ICore *rc = qscriptvalue_cast<ICore *>(thisObject());
    QTC_ASSERT(rc, return 0);
    return rc;
}

// -- MessageManager

MessageManagerPrototype::MessageManagerPrototype(QObject *parent) :
    QObject(parent)
{
}

void MessageManagerPrototype::displayStatusBarMessage(const QString &text, int ms)
{
    MessageManager *mm = qscriptvalue_cast<MessageManager *>(thisObject());
    QTC_ASSERT(mm, return);
    mm->displayStatusBarMessage(text, ms);
}

void MessageManagerPrototype::printToOutputPane(const QString &text, bool bringToForeground)
{
    MessageManager *mm = qscriptvalue_cast<MessageManager *>(thisObject());
    QTC_ASSERT(mm, return);
    mm->printToOutputPane(text, bringToForeground);
}

QString MessageManagerPrototype::toString() const
{
    return QLatin1String("MessageManager");
}

// -- FileManagerPrototype

FileManagerPrototype::FileManagerPrototype(QObject *parent) :
    QObject(parent)
{
}

FileManager *FileManagerPrototype::callee() const
{
    FileManager *rc = qscriptvalue_cast<FileManager *>(thisObject());
    QTC_ASSERT(rc, return 0);
    return rc;
}

bool FileManagerPrototype::addFiles(const QList<Core::IFile *> &files)
{
    return callee()->addFiles(files);
}

bool FileManagerPrototype::addFile(Core::IFile *file)
{
    return callee()->addFile(file);
}

bool FileManagerPrototype::removeFile(Core::IFile *file)
{
    return callee()->removeFile(file);
}

QList<Core::IFile*>
FileManagerPrototype::saveModifiedFilesSilently(const QList<Core::IFile*> &files)
{
    return callee()->saveModifiedFilesSilently(files);
}

QString FileManagerPrototype::getSaveAsFileName(Core::IFile *file)
{
    return callee()->getSaveAsFileName(file);
}

bool FileManagerPrototype::isFileManaged(const QString &fileName) const
{
    return callee()->isFileManaged(fileName);
}

QList<Core::IFile *>
FileManagerPrototype::managedFiles(const QString &fileName) const
{
    return callee()->managedFiles(fileName);
}

void FileManagerPrototype::blockFileChange(Core::IFile *file)
{
    callee()->blockFileChange(file);
}

void FileManagerPrototype::unblockFileChange(Core::IFile *file)
{
    return callee()->unblockFileChange(file);
}

void FileManagerPrototype::addToRecentFiles(const QString &fileName)
{
    return callee()->addToRecentFiles(fileName);
}

QStringList FileManagerPrototype::recentFiles() const
{
    return callee()->recentFiles();
}

QString FileManagerPrototype::toString() const
{
    return QLatin1String("FileManager");
}

// ------- FilePrototype

FilePrototype::FilePrototype(QObject *parent) :
    QObject(parent)
{
}

IFile *FilePrototype::callee() const
{
    IFile *rc = qscriptvalue_cast<IFile *>(thisObject());
    QTC_ASSERT(rc, return 0);
    return rc;
}

QString FilePrototype::fileName () const { return callee()->fileName(); }
QString FilePrototype::defaultPath () const { return callee()->defaultPath(); }
QString FilePrototype::suggestedFileName () const { return callee()->suggestedFileName(); }

bool FilePrototype::isModified () const { return callee()->isModified(); }
bool FilePrototype::isReadOnly () const { return callee()->isReadOnly(); }
bool FilePrototype::isSaveAsAllowed () const { return callee()->isSaveAsAllowed(); }

QString FilePrototype::toString() const
{
    QString rc = QLatin1String("File(");
    rc += fileName();
    rc +=  QLatin1Char(')');
    return rc;
}

// ------------- EditorManagerPrototype

EditorManagerPrototype::EditorManagerPrototype(QObject *parent) :
    QObject(parent)
{
}

Core::IEditor *EditorManagerPrototype::currentEditor() const
{
    return callee()->currentEditor();
}

void EditorManagerPrototype::setCurrentEditor(Core::IEditor *editor)
{
    callee()->setCurrentEditor(editor);
}

QList<Core::IEditor*> EditorManagerPrototype::openedEditors() const
{
    return callee()->openedEditors();
}

QList<Core::IEditor*> EditorManagerPrototype::editorHistory() const
{
    return callee()->editorHistory();
}

QList<Core::EditorGroup *> EditorManagerPrototype::editorGroups() const
{
    return callee()->editorGroups();
}

QList<Core::IEditor*> EditorManagerPrototype::editorsForFiles(QList<Core::IFile*> files) const
{
    return callee()->editorsForFiles(files);
}

bool EditorManagerPrototype::closeEditors(const QList<Core::IEditor*> editorsToClose, bool askAboutModifiedEditors)
{
    return  callee()->closeEditors(editorsToClose, askAboutModifiedEditors);
}

Core::IEditor *EditorManagerPrototype::openEditor(const QString &fileName, const QString &editorKind)
{
    return callee()->openEditor(fileName, editorKind);
}

Core::IEditor *EditorManagerPrototype::newFile(const QString &editorKind, QString titlePattern, const QString &contents)
{
    return callee()->newFile(editorKind, &titlePattern, contents);
}

int EditorManagerPrototype::makeEditorWritable(Core::IEditor *editor)
{
    return callee()->makeEditorWritable(editor);
}

QString EditorManagerPrototype::toString() const
{
     return QLatin1String("EditorManager");
}

EditorManagerPrototype::EditorManager *EditorManagerPrototype::callee() const
{
    EditorManager *rc = qscriptvalue_cast<EditorManager *>(thisObject());
    QTC_ASSERT(rc, return 0);
    return rc;
}

// ------------- EditorPrototype

EditorPrototype::EditorPrototype(QObject *parent)
  : QObject(parent)
{
}

QString EditorPrototype::displayName() const
{
    return callee()->displayName();
}

void EditorPrototype::setDisplayName(const QString &title)
{
    callee()->setDisplayName(title);
}

QString EditorPrototype::kind() const
{
    return  QLatin1String(callee()->kind());
}

bool EditorPrototype::duplicateSupported() const
{
    return callee()->duplicateSupported();
}

bool EditorPrototype::createNew(const QString &contents)
{
    return callee()->createNew(contents);
}

bool EditorPrototype::open(const QString &fileName)
{
    return callee()->open(fileName);
}

Core::IEditor *EditorPrototype::duplicate(QWidget *parent)
{
    return callee()->duplicate(parent);
}

Core::IFile *EditorPrototype::file() const
{
    return callee()->file();
}

QToolBar* EditorPrototype::toolBar() const
{
    return callee()->toolBar();
}

Core::IEditor *EditorPrototype::callee() const
{
    IEditor *rc = qscriptvalue_cast<IEditor *>(thisObject());
    QTC_ASSERT(rc, return 0);
    return rc;
}

QString EditorPrototype::toString() const
{
    QString rc = QLatin1String("Editor(");
    rc += displayName();
    rc +=  QLatin1Char(')');
    return rc;
}

// ----------- EditorGroupPrototype

EditorGroupPrototype::EditorGroupPrototype(QObject *parent) :
    QObject(parent)
{
}

int EditorGroupPrototype::editorCount() const
{
    return callee()->editorCount();
}

Core::IEditor *EditorGroupPrototype::currentEditor() const
{
    return callee()->currentEditor();
}

void EditorGroupPrototype::setCurrentEditor(Core::IEditor *editor)
{
    callee()->setCurrentEditor(editor);
}

QList<Core::IEditor*> EditorGroupPrototype::editors() const
{
    return callee()->editors();
}

void EditorGroupPrototype::addEditor(Core::IEditor *editor)
{
    callee()->addEditor(editor);
}

void EditorGroupPrototype::insertEditor(int i, Core::IEditor *editor)
{
    callee()->insertEditor(i, editor);
}

void EditorGroupPrototype::removeEditor(Core::IEditor *editor)
{
    callee()->removeEditor(editor);
}


void EditorGroupPrototype::moveEditorsFromGroup(Core::EditorGroup *group)
{
    callee()->moveEditorsFromGroup(group);
}

void EditorGroupPrototype::moveEditorFromGroup(Core::EditorGroup *group, Core::IEditor *editor)
{
    callee()->moveEditorFromGroup(group, editor);
}

QString EditorGroupPrototype::toString() const
{
    return QLatin1String("EditorGroup");
}

Core::EditorGroup *EditorGroupPrototype::callee() const
{
    EditorGroup *rc = qscriptvalue_cast<EditorGroup *>(thisObject());
    QTC_ASSERT(rc, return 0);
    return rc;
}

} // namespace Internal
} // namespace Core
