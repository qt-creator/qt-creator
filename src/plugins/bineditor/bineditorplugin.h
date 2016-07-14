/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "bineditorservice.h"

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icontext.h>

#include <QPointer>
#include <QStringList>
#include <QAction>

namespace BinEditor {
namespace Internal {

class BinEditorWidget;
class BinEditorFactory;

class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

public:
    BinEditorPlugin();
    ~BinEditorPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

    // Connect editor to settings changed signals.
    void initializeEditor(BinEditorWidget *editor);

private:
    void undoAction();
    void redoAction();
    void copyAction();
    void selectAllAction();
    void updateActions();

    void updateCurrentEditor(Core::IEditor *editor);

    Core::Context m_context;
    QAction *registerNewAction(Core::Id id, const QString &title = QString());
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_selectAllAction = nullptr;

    QPointer<BinEditorWidget> m_currentEditor;
};

class BinEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    explicit BinEditorFactory(BinEditorPlugin *owner);

    Core::IEditor *createEditor();

private:
    BinEditorPlugin *m_owner;
};

class FactoryServiceImpl : public QObject, public FactoryService
{
    Q_OBJECT
    Q_INTERFACES(BinEditor::FactoryService)

public:
    EditorService *createEditorService(const QString &title0, bool wantsEditor) override;
};

} // namespace Internal
} // namespace BinEditor
