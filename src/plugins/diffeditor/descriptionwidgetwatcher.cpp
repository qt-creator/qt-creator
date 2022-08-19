// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
