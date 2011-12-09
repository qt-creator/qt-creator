/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <qrceditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <utils/reloadpromptutils.h>
#include <utils/fileutils.h>

#include <QtCore/QTemporaryFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/qdebug.h>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>

namespace ResourceEditor {
namespace Internal {

enum { debugResourceEditorW = 0 };



ResourceEditorFile::ResourceEditorFile(ResourceEditorW *parent) :
    IFile(parent),
    m_mimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE)),
    m_parent(parent)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

QString ResourceEditorFile::mimeType() const
{
    return m_mimeType;
}


ResourceEditorW::ResourceEditorW(const Core::Context &context,
                               ResourceEditorPlugin *plugin,
                               QWidget *parent)
      : m_resourceEditor(new SharedTools::QrcEditor(parent)),
        m_resourceFile(new ResourceEditorFile(this)),
        m_plugin(plugin),
        m_shouldAutoSave(false),
        m_diskIo(false),
        m_contextMenu(new QMenu)
{
    setContext(context);
    setWidget(m_resourceEditor);

    m_resourceEditor->setResourceDragEnabled(true);
    m_openWithMenu = m_contextMenu->addMenu(tr("Open With"));
    // Below we need QueuedConnection because otherwise, if this qrc file
    // is inside of the qrc file, crashes happen when using "Open With" on it.
    // (That is because this editor instance is deleted in executeOpenWithMenuAction
    // in that case.)
    connect(m_openWithMenu, SIGNAL(triggered(QAction*)),
            Core::FileManager::instance(), SLOT(executeOpenWithMenuAction(QAction*)),
            Qt::QueuedConnection);

    connect(m_resourceEditor, SIGNAL(dirtyChanged(bool)), this, SLOT(dirtyChanged(bool)));
    connect(m_resourceEditor, SIGNAL(undoStackChanged(bool, bool)),
            this, SLOT(onUndoStackChanged(bool, bool)));
    connect(m_resourceEditor, SIGNAL(showContextMenu(QPoint,QString)),
            this, SLOT(showContextMenu(QPoint,QString)));
    connect(m_resourceEditor->commandHistory(), SIGNAL(indexChanged(int)),
            this, SLOT(setShouldAutoSave()));
    connect(m_resourceFile, SIGNAL(changed()), this, SIGNAL(changed()));
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::ResourceEditorW()";
}

ResourceEditorW::~ResourceEditorW()
{
    if (m_resourceEditor)
        m_resourceEditor->deleteLater();
    delete m_contextMenu;
}

bool ResourceEditorW::createNew(const QString &contents)
{
    Utils::TempFileSaver saver;
    saver.write(contents.toUtf8());
    if (!saver.finalize(Core::ICore::instance()->mainWindow()))
        return false;

    const bool rc = m_resourceEditor->load(saver.fileName());
    m_resourceEditor->setFileName(QString());
    m_shouldAutoSave = false;
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << saver.fileName() << ") returns " << rc;
    return rc;
}

bool ResourceEditorW::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << fileName;

    if (fileName.isEmpty()) {
        setDisplayName(tr("untitled"));
        return true;
    }

    const QFileInfo fi(fileName);

    m_diskIo = true;
    if (!m_resourceEditor->load(realFileName)) {
        *errorString = m_resourceEditor->errorMessage();
        m_diskIo = false;
        return false;
    }

    m_resourceEditor->setFileName(fileName);
    m_resourceEditor->setDirty(fileName != realFileName);
    setDisplayName(fi.fileName());
    m_shouldAutoSave = false;
    m_diskIo = false;

    emit changed();
    return true;
}

bool ResourceEditorFile::save(QString *errorString, const QString &name, bool autoSave)
{
    if (debugResourceEditorW)
        qDebug(">ResourceEditorW::save: %s", qPrintable(name));

    const QString oldFileName = fileName();
    const QString actualName = name.isEmpty() ? oldFileName : name;
    if (actualName.isEmpty())
        return false;

    m_parent->m_diskIo = true;
    m_parent->m_resourceEditor->setFileName(actualName);
    if (!m_parent->m_resourceEditor->save()) {
        *errorString = m_parent->m_resourceEditor->errorMessage();
        m_parent->m_resourceEditor->setFileName(oldFileName);
        m_parent->m_diskIo = false;
        return false;
    }

    m_parent->m_shouldAutoSave = false;
    if (autoSave) {
        m_parent->m_resourceEditor->setFileName(oldFileName);
        m_parent->m_resourceEditor->setDirty(true);
        m_parent->m_diskIo = false;
        return true;
    }

    m_parent->setDisplayName(QFileInfo(actualName).fileName());
    m_parent->m_diskIo = false;

    emit changed();
    return true;
}

void ResourceEditorFile::rename(const QString &newName)
{
    m_parent->m_resourceEditor->setFileName(newName);
    emit changed();
}

Core::Id ResourceEditorW::id() const
{
    return Core::Id(ResourceEditor::Constants::RESOURCEEDITOR_ID);
}

QString ResourceEditorFile::fileName() const
{
    return m_parent->m_resourceEditor->fileName();
}

bool ResourceEditorFile::shouldAutoSave() const
{
    return m_parent->m_shouldAutoSave;
}

bool ResourceEditorFile::isModified() const
{
    return m_parent->m_resourceEditor->isDirty();
}

bool ResourceEditorFile::isReadOnly() const
{
    const QString fileName = m_parent->m_resourceEditor->fileName();
    if (fileName.isEmpty())
        return false;
    const QFileInfo fi(fileName);
    return !fi.isWritable();
}

bool ResourceEditorFile::isSaveAsAllowed() const
{
    return true;
}

bool ResourceEditorFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        emit changed();
    } else {
        emit aboutToReload();
        QString fn = m_parent->m_resourceEditor->fileName();
        if (!m_parent->open(errorString, fn, fn))
            return false;
        emit reloaded();
    }
    return true;
}

QString ResourceEditorFile::defaultPath() const
{
    return QString();
}

void ResourceEditorW::setSuggestedFileName(const QString &fileName)
{
    m_suggestedName = fileName;
}

QString ResourceEditorFile::suggestedFileName() const
{
    return m_parent->m_suggestedName;
}

void ResourceEditorW::dirtyChanged(bool dirty)
{
    if (m_diskIo)
        return; // We emit changed() afterwards, unless it was an autosave

    if (debugResourceEditorW)
        qDebug() << " ResourceEditorW::dirtyChanged" <<  dirty;
    emit changed();
}

void ResourceEditorW::onUndoStackChanged(bool canUndo, bool canRedo)
{
    m_plugin->onUndoStackChanged(this, canUndo, canRedo);
}

void ResourceEditorW::showContextMenu(const QPoint &globalPoint, const QString &fileName)
{
    Core::FileManager::populateOpenWithMenu(m_openWithMenu, fileName);
    if (!m_openWithMenu->actions().isEmpty())
        m_contextMenu->popup(globalPoint);
}

void ResourceEditorW::onUndo()
{
    if (!m_resourceEditor.isNull())
        m_resourceEditor.data()->onUndo();
}

void ResourceEditorW::onRedo()
{
    if (!m_resourceEditor.isNull())
        m_resourceEditor.data()->onRedo();
}

} // namespace Internal
} // namespace ResourceEditor
