/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qworkbench_wrapper.h"

#include <wrap_helpers.h>

#include <coreplugin/messagemanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QMainWindow>

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

void CorePrototype::updateAdditionalContexts(const QList<int> &remove, const QList<int> &add)
{
    callee()->updateAdditionalContexts(remove, add);
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

void MessageManagerPrototype::printToOutputPane(const QString &text, bool bringToForeground)
{
    MessageManager *mm = qscriptvalue_cast<MessageManager *>(thisObject());
    QTC_ASSERT(mm, return);
    mm->printToOutputPane(text, bringToForeground);
}

void MessageManagerPrototype::printToOutputPanePopup(const QString &text)
{
    MessageManager *mm = qscriptvalue_cast<MessageManager *>(thisObject());
    QTC_ASSERT(mm, return);
    mm->printToOutputPanePopup(text);
}

void MessageManagerPrototype::printToOutputPane(const QString &text)
{
    MessageManager *mm = qscriptvalue_cast<MessageManager *>(thisObject());
    QTC_ASSERT(mm, return);
    mm->printToOutputPane(text);
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

void EditorManagerPrototype::activateEditor(Core::IEditor *editor)
{
    callee()->activateEditor(editor);
}

QList<Core::IEditor*> EditorManagerPrototype::openedEditors() const
{
    return callee()->openedEditors();
}

QList<Core::IEditor*> EditorManagerPrototype::editorsForFiles(QList<Core::IFile*> files) const
{
    return callee()->editorsForFiles(files);
}

bool EditorManagerPrototype::closeEditors(const QList<Core::IEditor*> editorsToClose, bool askAboutModifiedEditors)
{
    return  callee()->closeEditors(editorsToClose, askAboutModifiedEditors);
}

Core::IEditor *EditorManagerPrototype::openEditor(const QString &fileName, const QString &editorId)
{
    return callee()->openEditor(fileName, editorId);
}

Core::IEditor *EditorManagerPrototype::newFile(const QString &editorId, QString titlePattern, const QString &contents)
{
    return callee()->openEditorWithContents(editorId, &titlePattern, contents);
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

QString EditorPrototype::id() const
{
    return  callee()->id();
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

QWidget* EditorPrototype::toolBar() const
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


} // namespace Internal
} // namespace Core
