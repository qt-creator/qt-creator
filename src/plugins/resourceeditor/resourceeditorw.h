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

#ifndef RESOURCEDITORW_H
#define RESOURCEDITORW_H

#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/ieditor.h>

QT_BEGIN_NAMESPACE
class QMenu;
class QToolBar;
QT_END_NAMESPACE

namespace ResourceEditor {
namespace Internal {

class ResourceEditorPlugin;
class ResourceEditorW;
class QrcEditor;

class ResourceEditorDocument
  : public Core::IDocument
{
    Q_OBJECT

public:
    ResourceEditorDocument(ResourceEditorW *parent = 0);

    //IDocument
    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;
    bool shouldAutoSave() const;
    bool isModified() const;
    bool isSaveAsAllowed() const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    QString defaultPath() const;
    QString suggestedFileName() const;
    virtual QString mimeType() const;
    virtual void rename(const QString &newName);

private:
    const QString m_mimeType;
    ResourceEditorW *m_parent;
};

class ResourceEditorW : public Core::IEditor
{
    Q_OBJECT

public:
    ResourceEditorW(const Core::Context &context,
                   ResourceEditorPlugin *plugin,
                   QWidget *parent = 0);
    ~ResourceEditorW();

    // IEditor
    bool createNew(const QString &contents);
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget *) { return 0; }
    Core::IDocument *document() { return m_resourceDocument; }
    Core::Id id() const;
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &title) { m_displayName = title; emit changed(); }
    QWidget *toolBar();
    QByteArray saveState() const { return QByteArray(); }
    bool restoreState(const QByteArray &/*state*/) { return true; }

    void setSuggestedFileName(const QString &fileName);
    bool isTemporary() const { return false; }

private slots:
    void dirtyChanged(bool);
    void onUndoStackChanged(bool canUndo, bool canRedo);
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }
    void showContextMenu(const QPoint &globalPoint, const QString &fileName);
    void openCurrentFile();
    void openFile(const QString &fileName);
    void renameCurrentFile();
    void copyCurrentResourcePath();

private:
    const QString m_extension;
    const QString m_fileFilter;
    QString m_displayName;
    QString m_suggestedName;
    QrcEditor *m_resourceEditor;
    ResourceEditorDocument *m_resourceDocument;
    ResourceEditorPlugin *m_plugin;
    bool m_shouldAutoSave;
    bool m_diskIo;
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

#endif // RESOURCEDITORW_H
