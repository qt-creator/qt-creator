/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef RESOURCEDITORW_H
#define RESOURCEDITORW_H

#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QPointer>

namespace SharedTools {
    class QrcEditor;
}

namespace ResourceEditor {
namespace Internal {

class ResourceEditorPlugin;
class ResourceEditorW;

class ResourceEditorFile
  : public virtual Core::IFile
{
    Q_OBJECT

public:
    ResourceEditorFile(ResourceEditorW *parent = 0);

    //IFile
    bool save(const QString &fileName = QString());
    QString fileName() const;
    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);
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
    bool open(const QString &fileName = QString());
    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget *) { return 0; }
    Core::IFile *file() { return m_resourceFile; }
    QString id() const;
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &title) { m_displayName = title; emit changed(); }
    QWidget *toolBar() { return 0; }
    QByteArray saveState() const { return QByteArray(); }
    bool restoreState(const QByteArray &/*state*/) { return true; }

    // ContextInterface
    Core::Context context() const { return m_context; }
    QWidget *widget();

    void setSuggestedFileName(const QString &fileName);
    bool isTemporary() const { return false; }

private slots:
    void dirtyChanged(bool);
    void onUndoStackChanged(bool canUndo, bool canRedo);

private:
    const QString m_extension;
    const QString m_fileFilter;
    QString m_displayName;
    QString m_suggestedName;
    const Core::Context m_context;
    QPointer<SharedTools::QrcEditor> m_resourceEditor;
    ResourceEditorFile *m_resourceFile;
    ResourceEditorPlugin *m_plugin;

public:
    void onUndo();
    void onRedo();

    friend class ResourceEditorFile;
};

} // namespace Internal
} // namespace ResourceEditor

#endif // RESOURCEDITORW_H
