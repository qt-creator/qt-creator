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

#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <qrceditor.h>

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <find/treeviewfind.h>
#include <utils/reloadpromptutils.h>
#include <utils/fileutils.h>

#include <QTemporaryFile>
#include <QFileInfo>
#include <QDir>
#include <qdebug.h>
#include <QHBoxLayout>
#include <QMenu>
#include <QToolBar>
#include <QInputDialog>
#include <QClipboard>

namespace ResourceEditor {
namespace Internal {

enum { debugResourceEditorW = 0 };



ResourceEditorDocument::ResourceEditorDocument(ResourceEditorW *parent) :
    IDocument(parent),
    m_mimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE)),
    m_parent(parent)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

QString ResourceEditorDocument::mimeType() const
{
    return m_mimeType;
}


ResourceEditorW::ResourceEditorW(const Core::Context &context,
                               ResourceEditorPlugin *plugin,
                               QWidget *parent)
      : m_resourceEditor(new QrcEditor(parent)),
        m_resourceDocument(new ResourceEditorDocument(this)),
        m_plugin(plugin),
        m_shouldAutoSave(false),
        m_diskIo(false),
        m_contextMenu(new QMenu),
        m_toolBar(new QToolBar)
{
    setContext(context);
    setWidget(m_resourceEditor);

    Core::CommandButton *refreshButton = new Core::CommandButton(Constants::REFRESH, m_toolBar);
    refreshButton->setIcon(QIcon(QLatin1String(":/texteditor/images/finddocuments.png")));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(onRefresh()));
    m_toolBar->addWidget(refreshButton);

    Aggregation::Aggregate * agg = new Aggregation::Aggregate;
    agg->add(m_resourceEditor->treeView());
    agg->add(new Find::TreeViewFind(m_resourceEditor->treeView()));

    m_resourceEditor->setResourceDragEnabled(true);
    m_contextMenu->addAction(tr("Open File"), this, SLOT(openCurrentFile()));
    m_openWithMenu = m_contextMenu->addMenu(tr("Open With"));
    m_renameAction = m_contextMenu->addAction(tr("Rename File..."), this, SLOT(renameCurrentFile()));
    m_copyFileNameAction = m_contextMenu->addAction(tr("Copy Resource Path to Clipboard"), this, SLOT(copyCurrentResourcePath()));

    // Below we need QueuedConnection because otherwise, if this qrc file
    // is inside of the qrc file, crashes happen when using "Open With" on it.
    // (That is because this editor instance is deleted in executeOpenWithMenuAction
    // in that case.)
    connect(m_openWithMenu, SIGNAL(triggered(QAction*)),
            Core::DocumentManager::instance(), SLOT(slotExecuteOpenWithMenuAction(QAction*)),
            Qt::QueuedConnection);

    connect(m_resourceEditor, SIGNAL(dirtyChanged(bool)), this, SLOT(dirtyChanged(bool)));
    connect(m_resourceEditor, SIGNAL(undoStackChanged(bool,bool)),
            this, SLOT(onUndoStackChanged(bool,bool)));
    connect(m_resourceEditor, SIGNAL(showContextMenu(QPoint,QString)),
            this, SLOT(showContextMenu(QPoint,QString)));
    connect(m_resourceEditor, SIGNAL(itemActivated(QString)),
            this, SLOT(openFile(QString)));
    connect(m_resourceEditor->commandHistory(), SIGNAL(indexChanged(int)),
            this, SLOT(setShouldAutoSave()));
    connect(m_resourceDocument, SIGNAL(changed()), this, SIGNAL(changed()));
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::ResourceEditorW()";
}

ResourceEditorW::~ResourceEditorW()
{
    if (m_resourceEditor)
        m_resourceEditor->deleteLater();
    delete m_contextMenu;
    delete m_toolBar;
}

bool ResourceEditorW::createNew(const QString &contents)
{
    Utils::TempFileSaver saver;
    saver.write(contents.toUtf8());
    if (!saver.finalize(Core::ICore::mainWindow()))
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

bool ResourceEditorDocument::save(QString *errorString, const QString &name, bool autoSave)
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

void ResourceEditorDocument::rename(const QString &newName)
{
    const QString oldName = m_parent->m_resourceEditor->fileName();
    m_parent->m_resourceEditor->setFileName(newName);
    emit fileNameChanged(oldName, newName); // TODO Are there other cases where the ressource file name changes?
    emit changed();
}

Core::Id ResourceEditorW::id() const
{
    return Core::Id(ResourceEditor::Constants::RESOURCEEDITOR_ID);
}

QWidget *ResourceEditorW::toolBar()
{
    return m_toolBar;
}

QString ResourceEditorDocument::fileName() const
{
    return m_parent->m_resourceEditor->fileName();
}

bool ResourceEditorDocument::shouldAutoSave() const
{
    return m_parent->m_shouldAutoSave;
}

bool ResourceEditorDocument::isModified() const
{
    return m_parent->m_resourceEditor->isDirty();
}

bool ResourceEditorDocument::isSaveAsAllowed() const
{
    return true;
}

bool ResourceEditorDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
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

QString ResourceEditorDocument::defaultPath() const
{
    return QString();
}

void ResourceEditorW::setSuggestedFileName(const QString &fileName)
{
    m_suggestedName = fileName;
}

QString ResourceEditorDocument::suggestedFileName() const
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
    Core::DocumentManager::populateOpenWithMenu(m_openWithMenu, fileName);
    m_currentFileName = fileName;
    m_renameAction->setEnabled(!document()->isFileReadOnly());
    m_contextMenu->popup(globalPoint);
}

void ResourceEditorW::openCurrentFile()
{
    openFile(m_currentFileName);
}

void ResourceEditorW::openFile(const QString &fileName)
{
    Core::EditorManager::openEditor(fileName);
}

void ResourceEditorW::onRefresh()
{
    m_resourceEditor->refresh();
}

void ResourceEditorW::renameCurrentFile()
{
    m_resourceEditor->editCurrentItem();
}

void ResourceEditorW::copyCurrentResourcePath()
{
    QApplication::clipboard()->setText(m_resourceEditor->currentResourcePath());
}

void ResourceEditorW::onUndo()
{
    m_resourceEditor->onUndo();
}

void ResourceEditorW::onRedo()
{
    m_resourceEditor->onRedo();
}

} // namespace Internal
} // namespace ResourceEditor
