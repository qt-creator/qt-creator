// Copyright (C) 2016 Marc Reilly <marc.reilly+qtc@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basehoverhandler.h>

#include <QString>

namespace CppEditor {
namespace Internal {

class ResourcePreviewHoverHandler : public TextEditor::BaseHoverHandler
{
private:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override;
    void operateTooltip(TextEditor::TextEditorWidget *editorWidget, const QPoint &point) override;

private:
    QString makeTooltip() const;

    QString m_resPath;
};

} // namespace Internal
} // namespace CppEditor
