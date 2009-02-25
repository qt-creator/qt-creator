/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef RESOURCEEDITORPLUGIN_H
#define RESOURCEEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ResourceEditor {
namespace Internal {

class ResourceEditorW;
class ResourceWizard;
class ResourceEditorFactory;

class ResourceEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    ResourceEditorPlugin();
    virtual ~ResourceEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

private slots:
    void onUndo();
    void onRedo();

public:
    void onUndoStackChanged(ResourceEditorW const *editor, bool canUndo, bool canRedo);

private:
    ResourceEditorW * currentEditor() const;

private:
    ResourceWizard *m_wizard;
    ResourceEditorFactory *m_editor;
    QAction *m_redoAction;
    QAction *m_undoAction;
};

} // namespace Internal
} // namespace ResourceEditor

#endif // RESOURCEEDITORPLUGIN_H
