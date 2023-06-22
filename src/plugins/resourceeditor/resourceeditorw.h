// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/ieditor.h>

QT_BEGIN_NAMESPACE
class QMenu;
class QToolBar;
QT_END_NAMESPACE

namespace ResourceEditor::Internal {

class RelativeResourceModel;
class ResourceEditorPlugin;
class ResourceEditorW;
class QrcEditor;

class ResourceEditorDocument
  : public Core::IDocument
{
    Q_OBJECT
    Q_PROPERTY(QString plainText READ plainText STORED false) // For access by code pasters

public:
    ResourceEditorDocument(QObject *parent = nullptr);

    //IDocument
    OpenResult open(QString *errorString, const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    QString plainText() const;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    void setFilePath(const Utils::FilePath &newName) override;
    void setBlockDirtyChanged(bool value);

    RelativeResourceModel *model() const;
    void setShouldAutoSave(bool save);

signals:
    void loaded(bool success);

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    void dirtyChanged(bool);

    RelativeResourceModel *m_model;
    bool m_blockDirtyChanged = false;
    bool m_shouldAutoSave = false;
};

class ResourceEditorW : public Core::IEditor
{
    Q_OBJECT

public:
    ResourceEditorW(const Core::Context &context,
                   ResourceEditorPlugin *plugin,
                   QWidget *parent = nullptr);
    ~ResourceEditorW() override;

    // IEditor
    Core::IDocument *document() const override { return m_resourceDocument; }
    QByteArray saveState() const override;
    void restoreState(const QByteArray &state) override;
    QWidget *toolBar() override;

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
    ResourceEditorPlugin *m_plugin;
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

} // ResourceEditor::Internal
