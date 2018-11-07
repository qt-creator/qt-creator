/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "descriptionwidgetwatcher.h"
#include "diffeditor.h"
#include "diffeditorcontroller.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditor.h>

using namespace Core;

namespace DiffEditor {

DescriptionWidgetWatcher::DescriptionWidgetWatcher(DiffEditorController *controller)
    : QObject(controller), m_document(controller->document())
{
    const QList<IEditor *> editors = DocumentModel::editorsForDocument(controller->document());
    for (auto *editor : editors) {
        if (TextEditor::TextEditorWidget *widget = descriptionWidget(editor))
            m_widgets.append(widget);
    }

    connect(EditorManager::instance(), &EditorManager::editorOpened, this,
            [this](IEditor *editor) {
        if (TextEditor::TextEditorWidget *widget = descriptionWidget(editor)) {
            m_widgets.append(widget);
            emit descriptionWidgetAdded(widget);
        }
    });

    connect(EditorManager::instance(), &EditorManager::editorAboutToClose, this,
            [this](IEditor *editor) {
        if (TextEditor::TextEditorWidget *widget = descriptionWidget(editor)) {
            emit descriptionWidgetRemoved(widget);
            m_widgets.removeAll(widget);
        }
    });
}

QList<TextEditor::TextEditorWidget *> DescriptionWidgetWatcher::descriptionWidgets() const
{
    return m_widgets;
}

TextEditor::TextEditorWidget *DescriptionWidgetWatcher::descriptionWidget(Core::IEditor *editor) const
{
    if (auto diffEditor = qobject_cast<const Internal::DiffEditor *>(editor)) {
        if (diffEditor->document() == m_document)
            return diffEditor->descriptionWidget();
    }
    return nullptr;
}

} // namespace DiffEditor
