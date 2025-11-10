// Copyright (C) 2016 Marc Reilly <marc.reilly+qtc@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectexplorer_export.h>
#include <texteditor/basehoverhandler.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT ResourcePreviewHoverHandler : public TextEditor::BaseHoverHandler
{
private:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override;
    void operateTooltip(TextEditor::TextEditorWidget *editorWidget, const QPoint &point) override;

private:
    QString makeTooltip() const;

    QString m_path;
};

} // namespace ProjectExplorer
