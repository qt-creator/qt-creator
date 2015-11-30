/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <resourceeditor/qrceditor/resourcefile_p.h>
#include <resourceeditor/qrceditor/qrceditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/editormanager/editormanager.h>
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

using namespace Utils;

namespace ResourceEditor {
namespace Internal {

enum { debugResourceEditorW = 0 };



ResourceEditorDocument::ResourceEditorDocument(QObject *parent) :
    IDocument(parent),
    m_model(new RelativeResourceModel(this))
{
    setId(ResourceEditor::Constants::RESOURCEEDITOR_ID);
    setMimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE));
    connect(m_model, &RelativeResourceModel::dirtyChanged,
            this, &ResourceEditorDocument::dirtyChanged);

    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

ResourceEditorW::ResourceEditorW(const Core::Context &context,
                               ResourceEditorPlugin *plugin,
                               QWidget *parent)
      : m_resourceDocument(new ResourceEditorDocument(this)),
        m_plugin(plugin),
        m_contextMenu(new QMenu),
        m_toolBar(new QToolBar)
{
    m_resourceEditor = new QrcEditor(m_resourceDocument->model(), parent);

    setContext(context);
    setWidget(m_resourceEditor);

    Core::CommandButton *refreshButton = new Core::CommandButton(Constants::REFRESH, m_toolBar);
    refreshButton->setIcon(QIcon(QLatin1String(":/texteditor/images/finddocuments.png")));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(onRefresh()));
    m_toolBar->addWidget(refreshButton);

    m_resourceEditor->setResourceDragEnabled(true);
    m_contextMenu->addAction(tr("Open File"), this, SLOT(openCurrentFile()));
    m_openWithMenu = m_contextMenu->addMenu(tr("Open With"));
    m_renameAction = m_contextMenu->addAction(tr("Rename File..."), this, SLOT(renameCurrentFile()));
    m_copyFileNameAction = m_contextMenu->addAction(tr("Copy Resource Path to Clipboard"), this, SLOT(copyCurrentResourcePath()));

    connect(m_resourceDocument, &ResourceEditorDocument::loaded,
            m_resourceEditor, &QrcEditor::loaded);
    connect(m_resourceEditor, SIGNAL(undoStackChanged(bool,bool)),
            this, SLOT(onUndoStackChanged(bool,bool)));
    connect(m_resourceEditor, SIGNAL(showContextMenu(QPoint,QString)),
            this, SLOT(showContextMenu(QPoint,QString)));
    connect(m_resourceEditor, SIGNAL(itemActivated(QString)),
            this, SLOT(openFile(QString)));
    connect(m_resourceEditor->commandHistory(), &QUndoStack::indexChanged,
            m_resourceDocument, [this]() { m_resourceDocument->setShouldAutoSave(true); });
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

Core::IDocument::OpenResult ResourceEditorDocument::open(QString *errorString,
                                                         const QString &fileName,
                                                         const QString &realFileName)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << fileName;

    setBlockDirtyChanged(true);

    m_model->setFileName(realFileName);

    OpenResult openResult = m_model->reload();
    if (openResult != OpenResult::Success) {
        *errorString = m_model->errorMessage();
        setBlockDirtyChanged(false);
        emit loaded(false);
        return openResult;
    }

    setFilePath(FileName::fromString(fileName));
    setBlockDirtyChanged(false);
    m_model->setDirty(fileName != realFileName);
    m_shouldAutoSave = false;

    emit loaded(true);
    return OpenResult::Success;
}

bool ResourceEditorDocument::save(QString *errorString, const QString &name, bool autoSave)
{
    if (debugResourceEditorW)
        qDebug(">ResourceEditorW::save: %s", qPrintable(name));

    const FileName oldFileName = filePath();
    const FileName actualName = name.isEmpty() ? oldFileName : FileName::fromString(name);
    if (actualName.isEmpty())
        return false;

    m_blockDirtyChanged = true;
    m_model->setFileName(actualName.toString());
    if (!m_model->save()) {
        *errorString = m_model->errorMessage();
        m_model->setFileName(oldFileName.toString());
        m_blockDirtyChanged = false;
        return false;
    }

    m_shouldAutoSave = false;
    if (autoSave) {
        m_model->setFileName(oldFileName.toString());
        m_model->setDirty(true);
        m_blockDirtyChanged = false;
        return true;
    }

    setFilePath(actualName);
    m_blockDirtyChanged = false;

    emit changed();
    return true;
}

QString ResourceEditorDocument::plainText() const
{
    return m_model->contents();
}

bool ResourceEditorDocument::setContents(const QByteArray &contents)
{
    TempFileSaver saver;
    saver.write(contents);
    if (!saver.finalize(Core::ICore::mainWindow()))
        return false;

    const QString originalFileName = m_model->fileName();
    m_model->setFileName(saver.fileName());
    const bool success = (m_model->reload() == OpenResult::Success);
    m_model->setFileName(originalFileName);
    m_shouldAutoSave = false;
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << saver.fileName() << ") returns " << success;
    emit loaded(success);
    return success;
}

void ResourceEditorDocument::setFilePath(const FileName &newName)
{
    m_model->setFileName(newName.toString());
    IDocument::setFilePath(newName);
}

void ResourceEditorDocument::setBlockDirtyChanged(bool value)
{
    m_blockDirtyChanged = value;
}

RelativeResourceModel *ResourceEditorDocument::model() const
{
    return m_model;
}

void ResourceEditorDocument::setShouldAutoSave(bool save)
{
    m_shouldAutoSave = save;
}

QWidget *ResourceEditorW::toolBar()
{
    return m_toolBar;
}

bool ResourceEditorDocument::shouldAutoSave() const
{
    return m_shouldAutoSave;
}

bool ResourceEditorDocument::isModified() const
{
    return m_model->dirty();
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
        QString fn = filePath().toString();
        const bool success = (open(errorString, fn, fn) == OpenResult::Success);
        emit reloadFinished(success);
        return success;
    }
    return true;
}

QString ResourceEditorDocument::defaultPath() const
{
    return QString();
}

QString ResourceEditorDocument::suggestedFileName() const
{
    return QString();
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
    Core::EditorManager::populateOpenWithMenu(m_openWithMenu, fileName);
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
