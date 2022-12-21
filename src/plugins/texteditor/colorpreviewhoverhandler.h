// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"
#include "basehoverhandler.h"

#include <QColor>

namespace Core { class IEditor; }

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT ColorPreviewHoverHandler : public BaseHoverHandler
{
private:
    void identifyMatch(TextEditorWidget *editorWidget, int pos, ReportPriority report) override;
    void operateTooltip(TextEditorWidget *editorWidget, const QPoint &point) override;

    QColor m_colorTip;
};

} // namespace TextEditor
