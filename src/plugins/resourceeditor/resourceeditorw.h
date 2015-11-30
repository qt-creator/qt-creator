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

#ifndef RESOURCEEDITORW_H
#define RESOURCEEDITORW_H

#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/ieditor.h>

QT_BEGIN_NAMESPACE
class QMenu;
class QToolBar;
QT_END_NAMESPACE

namespace ResourceEditor {
namespace Internal {

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
    ResourceEditorDocument(QObject *parent = 0);

    //IDocument
    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    QString plainText() const;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    QString defaultPath() const override;
    QString suggestedFileName() const override;
    void setFilePath(const Utils::FileName &newName) override;
    void setBlockDirtyChanged(bool value);

    RelativeResourceModel *model() const;
    void setShouldAutoSave(bool save);

signals:
    void loaded(bool success);

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
                   QWidget *parent = 0);
    ~ResourceEditorW() override;

    // IEditor
    Core::IDocument *document() override { return m_resourceDocument; }
    QWidget *toolBar() override;

private slots:
    void onUndoStackChanged(bool canUndo, bool canRedo);
    void showContextMenu(const QPoint &globalPoint, const QString &fileName);
    void openCurrentFile();
    void openFile(const QString &fileName);
    void renameCurrentFile();
    void copyCurrentResourcePath();

private:
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

public slots:
    void onRefresh();

public:
    void onUndo();
    void onRedo();

    friend class ResourceEditorDocument;
};

} // namespace Internal
} // namespace ResourceEditor

#endif // RESOURCEEDITORW_H
