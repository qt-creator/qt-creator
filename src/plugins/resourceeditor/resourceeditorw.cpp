// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourceeditorw.h"

#include "resourceeditorconstants.h"
#include "resourceeditorplugin.h"
#include "resourceeditortr.h"
#include "qrceditor/qrceditor.h"
#include "qrceditor/resourcefile_p.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/reloadpromptutils.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QMenu>
#include <QToolBar>

using namespace Utils;

namespace ResourceEditor::Internal {

enum { debugResourceEditorW = 0 };

ResourceEditorDocument::ResourceEditorDocument(QObject *parent) :
    IDocument(parent),
    m_model(new RelativeResourceModel(this))
{
    setId(ResourceEditor::Constants::RESOURCEEDITOR_ID);
    setMimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE));
    connect(m_model, &RelativeResourceModel::dirtyChanged,
            this, &ResourceEditorDocument::dirtyChanged);
    connect(m_model, &ResourceModel::contentsChanged,
            this, &IDocument::contentsChanged);

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
    connect(refreshButton, &QAbstractButton::clicked, this, &ResourceEditorW::onRefresh);
    m_toolBar->addWidget(refreshButton);

    m_resourceEditor->setResourceDragEnabled(true);
    m_contextMenu->addAction(Tr::tr("Open File"), this, &ResourceEditorW::openCurrentFile);
    m_openWithMenu = m_contextMenu->addMenu(Tr::tr("Open With"));
    m_renameAction = m_contextMenu->addAction(Tr::tr("Rename File..."), this,
                                              &ResourceEditorW::renameCurrentFile);
    m_copyFileNameAction = m_contextMenu->addAction(Tr::tr("Copy Resource Path to Clipboard"),
                                                    this, &ResourceEditorW::copyCurrentResourcePath);
    m_orderList = m_contextMenu->addAction(Tr::tr("Sort Alphabetically"), this, &ResourceEditorW::orderList);

    connect(m_resourceDocument, &ResourceEditorDocument::loaded,
            m_resourceEditor, &QrcEditor::loaded);
    connect(m_resourceEditor, &QrcEditor::undoStackChanged,
            this, &ResourceEditorW::onUndoStackChanged);
    connect(m_resourceEditor, &QrcEditor::showContextMenu,
            this, &ResourceEditorW::showContextMenu);
    connect(m_resourceEditor, &QrcEditor::itemActivated,
            this, &ResourceEditorW::openFile);
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
                                                         const FilePath &filePath,
                                                         const FilePath &realFilePath)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << filePath;

    setBlockDirtyChanged(true);

    m_model->setFilePath(realFilePath);

    OpenResult openResult = m_model->reload();
    if (openResult != OpenResult::Success) {
        *errorString = m_model->errorMessage();
        setBlockDirtyChanged(false);
        emit loaded(false);
        return openResult;
    }

    setFilePath(filePath);
    setBlockDirtyChanged(false);
    m_model->setDirty(filePath != realFilePath);
    m_shouldAutoSave = false;

    emit loaded(true);
    return OpenResult::Success;
}

bool ResourceEditorDocument::saveImpl(QString *errorString, const FilePath &filePath, bool autoSave)
{
    if (debugResourceEditorW)
        qDebug() << ">ResourceEditorW::saveImpl: " << filePath;

    if (filePath.isEmpty())
        return false;

    m_blockDirtyChanged = true;
    m_model->setFilePath(filePath);
    if (!m_model->save()) {
        *errorString = m_model->errorMessage();
        m_model->setFilePath(this->filePath());
        m_blockDirtyChanged = false;
        return false;
    }

    m_shouldAutoSave = false;
    if (autoSave) {
        m_model->setFilePath(this->filePath());
        m_model->setDirty(true);
        m_blockDirtyChanged = false;
        return true;
    }

    setFilePath(filePath);
    m_blockDirtyChanged = false;

    emit changed();
    return true;
}

QString ResourceEditorDocument::plainText() const
{
    return m_model->contents();
}

QByteArray ResourceEditorDocument::contents() const
{
    return m_model->contents().toUtf8();
}

bool ResourceEditorDocument::setContents(const QByteArray &contents)
{
    TempFileSaver saver;
    saver.write(contents);
    if (!saver.finalize(Core::ICore::dialogParent()))
        return false;

    const FilePath originalFileName = m_model->filePath();
    m_model->setFilePath(saver.filePath());
    const bool success = (m_model->reload() == OpenResult::Success);
    m_model->setFilePath(originalFileName);
    m_shouldAutoSave = false;
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << saver.filePath() << ") returns " << success;
    emit loaded(success);
    return success;
}

void ResourceEditorDocument::setFilePath(const FilePath &newName)
{
    m_model->setFilePath(newName);
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

QByteArray ResourceEditorW::saveState() const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << m_resourceEditor->saveState();
    return bytes;
}

void ResourceEditorW::restoreState(const QByteArray &state)
{
    QDataStream stream(state);
    QByteArray splitterState;
    stream >> splitterState;
    m_resourceEditor->restoreState(splitterState);
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
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return true;
    emit aboutToReload();
    const bool success = (open(errorString, filePath(), filePath()) == OpenResult::Success);
    emit reloadFinished(success);
    return success;
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
    Core::EditorManager::populateOpenWithMenu(m_openWithMenu, FilePath::fromString(fileName));
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
    Core::EditorManager::openEditor(FilePath::fromString(fileName));
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
    setClipboardAndSelection(m_resourceEditor->currentResourcePath());
}

void ResourceEditorW::orderList()
{
    m_resourceDocument->model()->orderList();
}

void ResourceEditorW::onUndo()
{
    m_resourceEditor->onUndo();
}

void ResourceEditorW::onRedo()
{
    m_resourceEditor->onRedo();
}

} // ResourceEditor::Internal
