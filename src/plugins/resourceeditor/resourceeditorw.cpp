/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <resourceeditor/qrceditor/qrceditor.h>

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/find/treeviewfind.h>
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
    m_blockDirtyChanged(false),
    m_parent(parent)
{
    setId(ResourceEditor::Constants::RESOURCEEDITOR_ID);
    setMimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE));
    setFilePath(parent->m_resourceEditor->fileName());
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

ResourceEditorW::ResourceEditorW(const Core::Context &context,
                               ResourceEditorPlugin *plugin,
                               QWidget *parent)
      : m_resourceEditor(new QrcEditor(parent)),
        m_resourceDocument(new ResourceEditorDocument(this)),
        m_plugin(plugin),
        m_shouldAutoSave(false),
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
    agg->add(new Core::TreeViewFind(m_resourceEditor->treeView()));

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
            Core::DocumentManager::instance(), SLOT(executeOpenWithMenuAction(QAction*)),
            Qt::QueuedConnection);

    connect(m_resourceEditor, SIGNAL(dirtyChanged(bool)), m_resourceDocument, SLOT(dirtyChanged(bool)));
    connect(m_resourceEditor, SIGNAL(undoStackChanged(bool,bool)),
            this, SLOT(onUndoStackChanged(bool,bool)));
    connect(m_resourceEditor, SIGNAL(showContextMenu(QPoint,QString)),
            this, SLOT(showContextMenu(QPoint,QString)));
    connect(m_resourceEditor, SIGNAL(itemActivated(QString)),
            this, SLOT(openFile(QString)));
    connect(m_resourceEditor->commandHistory(), SIGNAL(indexChanged(int)),
            this, SLOT(setShouldAutoSave()));
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

bool ResourceEditorW::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << fileName;

    if (fileName.isEmpty())
        return true;

    m_resourceDocument->setBlockDirtyChanged(true);
    if (!m_resourceEditor->load(realFileName)) {
        *errorString = m_resourceEditor->errorMessage();
        m_resourceDocument->setBlockDirtyChanged(false);
        return false;
    }

    m_resourceDocument->setFilePath(fileName);
    m_resourceDocument->setBlockDirtyChanged(false);
    m_resourceEditor->setDirty(fileName != realFileName);
    m_shouldAutoSave = false;

    return true;
}

bool ResourceEditorDocument::save(QString *errorString, const QString &name, bool autoSave)
{
    if (debugResourceEditorW)
        qDebug(">ResourceEditorW::save: %s", qPrintable(name));

    const QString oldFileName = filePath();
    const QString actualName = name.isEmpty() ? oldFileName : name;
    if (actualName.isEmpty())
        return false;

    m_blockDirtyChanged = true;
    m_parent->m_resourceEditor->setFileName(actualName);
    if (!m_parent->m_resourceEditor->save()) {
        *errorString = m_parent->m_resourceEditor->errorMessage();
        m_parent->m_resourceEditor->setFileName(oldFileName);
        m_blockDirtyChanged = false;
        return false;
    }

    m_parent->m_shouldAutoSave = false;
    if (autoSave) {
        m_parent->m_resourceEditor->setFileName(oldFileName);
        m_parent->m_resourceEditor->setDirty(true);
        m_blockDirtyChanged = false;
        return true;
    }

    setFilePath(actualName);
    m_blockDirtyChanged = false;

    emit changed();
    return true;
}

bool ResourceEditorDocument::setContents(const QByteArray &contents)
{
    Utils::TempFileSaver saver;
    saver.write(contents);
    if (!saver.finalize(Core::ICore::mainWindow()))
        return false;

    const bool rc = m_parent->m_resourceEditor->load(saver.fileName());
    m_parent->m_shouldAutoSave = false;
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << saver.fileName() << ") returns " << rc;
    return rc;
}

void ResourceEditorDocument::setFilePath(const QString &newName)
{
    if (newName != m_parent->m_resourceEditor->fileName())
        m_parent->m_resourceEditor->setFileName(newName);
    IDocument::setFilePath(newName);
}

void ResourceEditorDocument::setBlockDirtyChanged(bool value)
{
    m_blockDirtyChanged = value;
}

QWidget *ResourceEditorW::toolBar()
{
    return m_toolBar;
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
        QString fn = filePath();
        const bool success = m_parent->open(errorString, fn, fn);
        emit reloadFinished(success);
        return success;
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

void ResourceEditorDocument::dirtyChanged(bool dirty)
{
    if (m_blockDirtyChanged)
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
