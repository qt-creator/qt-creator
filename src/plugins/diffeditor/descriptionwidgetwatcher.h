// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"

#include <QObject>

namespace Core {
class IDocument;
class IEditor;
}
namespace TextEditor { class TextEditorWidget; }

namespace DiffEditor {

class DiffEditorController;

class DIFFEDITOR_EXPORT DescriptionWidgetWatcher : public QObject
{
    Q_OBJECT
public:
    explicit DescriptionWidgetWatcher(DiffEditorController *controller);
    QList<TextEditor::TextEditorWidget *> descriptionWidgets() const;

signals:
    void descriptionWidgetAdded(TextEditor::TextEditorWidget *editor);
    void descriptionWidgetRemoved(TextEditor::TextEditorWidget *editor);

private:
    TextEditor::TextEditorWidget *descriptionWidget(Core::IEditor *editor) const;

    QList<TextEditor::TextEditorWidget *> m_widgets;
    Core::IDocument *m_document = nullptr;
};

} // namespace DiffEditor
