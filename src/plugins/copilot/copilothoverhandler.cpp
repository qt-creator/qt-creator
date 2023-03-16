// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilothoverhandler.h"

#include "copilotclient.h"
#include "copilotsuggestion.h"
#include "copilottr.h"

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>

#include <QPushButton>
#include <QToolBar>
#include <QToolButton>

using namespace TextEditor;
using namespace LanguageServerProtocol;
using namespace Utils;

namespace Copilot::Internal {

class CopilotCompletionToolTip : public QToolBar
{
public:
    CopilotCompletionToolTip(QList<Completion> completions,
                             int currentCompletion,
                             TextEditorWidget *editor)
        : m_numberLabel(new QLabel)
        , m_completions(completions)
        , m_currentCompletion(std::max(0, std::min<int>(currentCompletion, completions.size() - 1)))
        , m_editor(editor)
    {
        auto prev = addAction(Utils::Icons::PREV_TOOLBAR.icon(),
                              Tr::tr("Select Previous Copilot Suggestion"));
        prev->setEnabled(m_completions.size() > 1);
        addWidget(m_numberLabel);
        auto next = addAction(Utils::Icons::NEXT_TOOLBAR.icon(),
                              Tr::tr("Select Next Copilot Suggestion"));
        next->setEnabled(m_completions.size() > 1);

        auto apply = addAction(Tr::tr("Apply (Tab)"));

        connect(prev, &QAction::triggered, this, &CopilotCompletionToolTip::selectPrevious);
        connect(next, &QAction::triggered, this, &CopilotCompletionToolTip::selectNext);
        connect(apply, &QAction::triggered, this, &CopilotCompletionToolTip::apply);

        updateLabels();
    }

private:
    void updateLabels()
    {
        m_numberLabel->setText(Tr::tr("%1 of %2")
                                   .arg(m_currentCompletion + 1)
                                   .arg(m_completions.count()));
    }

    void selectPrevious()
    {
        --m_currentCompletion;
        if (m_currentCompletion < 0)
            m_currentCompletion = m_completions.size() - 1;
        setCurrentCompletion();
    }

    void selectNext()
    {
        ++m_currentCompletion;
        if (m_currentCompletion >= m_completions.size())
            m_currentCompletion = 0;
        setCurrentCompletion();
    }

    void setCurrentCompletion()
    {
        updateLabels();
        if (TextSuggestion *suggestion = m_editor->currentSuggestion())
            suggestion->reset();
        m_editor->insertSuggestion(std::make_unique<CopilotSuggestion>(m_completions,
                                                                       m_editor->document(),
                                                                       m_currentCompletion));
    }

    void apply()
    {
        if (TextSuggestion *suggestion = m_editor->currentSuggestion())
            suggestion->apply();
        ToolTip::hide();
    }

    QLabel *m_numberLabel;
    QList<Completion> m_completions;
    int m_currentCompletion = 0;
    TextEditorWidget *m_editor;
};

void CopilotHoverHandler::identifyMatch(TextEditorWidget *editorWidget,
                                        int pos,
                                        ReportPriority report)
{
    auto reportNone = qScopeGuard([&] { report(Priority_None); });
    if (!editorWidget->suggestionVisible())
        return;

    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(pos);
    m_block = cursor.block();
    auto *suggestion = dynamic_cast<CopilotSuggestion *>(TextDocumentLayout::suggestion(m_block));

    if (!suggestion)
        return;

    const QList<Completion> completions = suggestion->completions();
    if (completions.isEmpty())
        return;

    reportNone.dismiss();
    report(Priority_Suggestion);
}

void CopilotHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    auto *suggestion = dynamic_cast<CopilotSuggestion *>(TextDocumentLayout::suggestion(m_block));

    if (!suggestion)
        return;

    auto tooltipWidget = new CopilotCompletionToolTip(suggestion->completions(),
                                                      suggestion->currentCompletion(),
                                                      editorWidget);
    const qreal deltay = 2 * editorWidget->textDocument()->fontSettings().lineSpacing();
    ToolTip::show(point - QPoint{0, int(deltay)}, tooltipWidget, editorWidget);
}

} // namespace Copilot::Internal
