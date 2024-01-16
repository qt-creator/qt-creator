// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourceeditorw.h"

#include "projectexplorer/projectexplorerconstants.h"
#include "resourceeditorconstants.h"
#include "resourceeditortr.h"
#include "qrceditor/qrceditor.h"
#include "qrceditor/resourcefile_p.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/mimeconstants.h>
#include <utils/reloadpromptutils.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QToolBar>

using namespace Core;
using namespace Utils;

namespace ResourceEditor::Internal {

enum { debugResourceEditorW = 0 };

static QAction *s_redoAction = nullptr;
static QAction *s_undoAction = nullptr;
static QAction *s_refreshAction = nullptr;

class ResourceEditorDocument final : public IDocument
{
    Q_OBJECT
    Q_PROPERTY(QString plainText READ plainText STORED false) // For access by code pasters

public:
    ResourceEditorDocument(QObject *parent = nullptr);

    OpenResult open(QString *errorString, const FilePath &filePath,
                    const FilePath &realFilePath) final;
    QString plainText() const;
    QByteArray contents() const final;
    bool setContents(const QByteArray &contents) final;
    bool shouldAutoSave() const final { return m_shouldAutoSave; }
    bool isModified() const final { return m_model.dirty(); }
    bool isSaveAsAllowed() const final { return true; }
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) final;
    void setFilePath(const FilePath &newName) final;
    void setBlockDirtyChanged(bool value) { m_blockDirtyChanged = value; }

    RelativeResourceModel *model() { return &m_model; }
    void setShouldAutoSave(bool save) { m_shouldAutoSave = save; }

signals:
    void loaded(bool success);

private:
    bool saveImpl(QString *errorString, const FilePath &filePath, bool autoSave) final;
    void dirtyChanged(bool);

    RelativeResourceModel m_model;
    bool m_blockDirtyChanged = false;
    bool m_shouldAutoSave = false;
};

ResourceEditorDocument::ResourceEditorDocument(QObject *parent)
    : IDocument(parent)
{
    setId(ResourceEditor::Constants::RESOURCEEDITOR_ID);
    setMimeType(Utils::Constants::RESOURCE_MIMETYPE);
    connect(&m_model, &RelativeResourceModel::dirtyChanged,
            this, &ResourceEditorDocument::dirtyChanged);
    connect(&m_model, &ResourceModel::contentsChanged,
            this, &IDocument::contentsChanged);

    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

class ResourceEditorW final : public IEditor
{
    Q_OBJECT

public:
    ResourceEditorW();
    ~ResourceEditorW() final;

    static ResourceEditorW *currentEditor()
    {
        auto const focusEditor = qobject_cast<ResourceEditorW *>(EditorManager::currentEditor());
        QTC_ASSERT(focusEditor, return nullptr);
        return focusEditor;
    }

    IDocument *document() const final { return m_resourceDocument; }
    QWidget *toolBar() final { return m_toolBar; }
    QByteArray saveState() const final;
    void restoreState(const QByteArray &state) final;

private:
    void onUndoStackChanged(bool canUndo, bool canRedo);
    void showContextMenu(const QPoint &globalPoint, const QString &fileName);
    void openCurrentFile();
    void openFile(const QString &fileName);
    void renameCurrentFile();
    void copyCurrentResourcePath();
    void orderList();

    const QString m_extension;
    const QString m_fileFilter;
    QString m_displayName;
    QrcEditor *m_resourceEditor;
    ResourceEditorDocument *m_resourceDocument;
    QMenu *m_contextMenu;
    QMenu *m_openWithMenu;
    QString m_currentFileName;
    QToolBar *m_toolBar;
    QAction *m_renameAction;
    QAction *m_copyFileNameAction;
    QAction *m_orderList;

public:
    void onRefresh();
    void onUndo();
    void onRedo();

    friend class ResourceEditorDocument;
};

ResourceEditorW::ResourceEditorW()
    : m_resourceDocument(new ResourceEditorDocument(this)),
    m_contextMenu(new QMenu),
    m_toolBar(new QToolBar)
{
    m_resourceEditor = new QrcEditor(m_resourceDocument->model(), nullptr);

    setContext(Context(Constants::C_RESOURCEEDITOR));
    setWidget(m_resourceEditor);

    CommandButton *refreshButton = new CommandButton(Constants::REFRESH, m_toolBar);
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
            m_resourceDocument, [this] { m_resourceDocument->setShouldAutoSave(true); });
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

IDocument::OpenResult ResourceEditorDocument::open(QString *errorString,
                                                   const FilePath &filePath,
                                                   const FilePath &realFilePath)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << filePath;

    setBlockDirtyChanged(true);

    m_model.setFilePath(realFilePath);

    OpenResult openResult = m_model.reload();
    if (openResult != OpenResult::Success) {
        *errorString = m_model.errorMessage();
        setBlockDirtyChanged(false);
        emit loaded(false);
        return openResult;
    }

    setFilePath(filePath);
    setBlockDirtyChanged(false);
    m_model.setDirty(filePath != realFilePath);
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
    m_model.setFilePath(filePath);
    if (!m_model.save()) {
        *errorString = m_model.errorMessage();
        m_model.setFilePath(this->filePath());
        m_blockDirtyChanged = false;
        return false;
    }

    m_shouldAutoSave = false;
    if (autoSave) {
        m_model.setFilePath(this->filePath());
        m_model.setDirty(true);
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
    return m_model.contents();
}

QByteArray ResourceEditorDocument::contents() const
{
    return m_model.contents().toUtf8();
}

bool ResourceEditorDocument::setContents(const QByteArray &contents)
{
    TempFileSaver saver;
    saver.write(contents);
    if (!saver.finalize(ICore::dialogParent()))
        return false;

    const FilePath originalFileName = m_model.filePath();
    m_model.setFilePath(saver.filePath());
    const bool success = (m_model.reload() == OpenResult::Success);
    m_model.setFilePath(originalFileName);
    m_shouldAutoSave = false;
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << saver.filePath() << ") returns " << success;
    emit loaded(success);
    return success;
}

void ResourceEditorDocument::setFilePath(const FilePath &newName)
{
    m_model.setFilePath(newName);
    IDocument::setFilePath(newName);
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
    if (currentEditor() == this) {
        s_undoAction->setEnabled(canUndo);
        s_redoAction->setEnabled(canRedo);
    }
}

void ResourceEditorW::showContextMenu(const QPoint &globalPoint, const QString &fileName)
{
    EditorManager::populateOpenWithMenu(m_openWithMenu, FilePath::fromString(fileName));
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
    EditorManager::openEditor(FilePath::fromString(fileName));
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

class ResourceEditorFactory final : public IEditorFactory
{
public:
    ResourceEditorFactory()
    {
        setId(Constants::RESOURCEEDITOR_ID);
        setMimeTypes(QStringList(Utils::Constants::RESOURCE_MIMETYPE));
        setDisplayName(::Core::Tr::tr(Constants::C_RESOURCEEDITOR_DISPLAY_NAME));

        FileIconProvider::registerIconOverlayForSuffix(
            ProjectExplorer::Constants::FILEOVERLAY_QRC, "qrc");

        setEditorCreator([] { return new ResourceEditorW; });
    }
};

void setupResourceEditor(QObject *guard)
{
    static ResourceEditorFactory theResourceEditorFactory;

    // Register undo and redo
    const Context context(Constants::C_RESOURCEEDITOR);

    s_undoAction = new QAction(Tr::tr("&Undo"), guard);
    s_redoAction = new QAction(Tr::tr("&Redo"), guard);
    s_refreshAction = new QAction(Tr::tr("Recheck Existence of Referenced Files"), guard);
    ActionManager::registerAction(s_undoAction, Core::Constants::UNDO, context);
    ActionManager::registerAction(s_redoAction, Core::Constants::REDO, context);
    ActionManager::registerAction(s_refreshAction, Constants::REFRESH, context);

    QObject::connect(s_undoAction, &QAction::triggered, guard, [] {
        if (ResourceEditorW *editor = ResourceEditorW::currentEditor())
            editor->onUndo();
    });

    QObject::connect(s_redoAction, &QAction::triggered, guard, [] {
        if (ResourceEditorW *editor = ResourceEditorW::currentEditor())
            editor->onRedo();
    });

    QObject::connect(s_refreshAction, &QAction::triggered, guard, [] {
        if (ResourceEditorW *editor = ResourceEditorW::currentEditor())
            editor->onRefresh();
    });
}

} // ResourceEditor::Internal

#include "resourceeditorw.moc"
