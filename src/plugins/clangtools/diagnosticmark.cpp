// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diagnosticmark.h"

#include "clangtoolsconstants.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"

#include <texteditor/textdocument.h>
#include <utils/utilsicons.h>
#include <utils/stringutils.h>

#include <QAction>

using namespace TextEditor;
using namespace Utils;

namespace ClangTools {
namespace Internal {

DiagnosticMark::DiagnosticMark(const Diagnostic &diagnostic, TextDocument *document)
    : TextMark(document,
               diagnostic.location.line,
               {Tr::tr("Clang Tools"), Id(Constants::DIAGNOSTIC_MARK_ID)})
    , m_diagnostic(diagnostic)
{
    setSettingsPage(Constants::SETTINGS_PAGE_ID);

    const bool isError = diagnostic.type == "error" || diagnostic.type == "fatal";
    setColor(isError ? Theme::CodeModel_Error_TextMarkColor : Theme::CodeModel_Warning_TextMarkColor);
    setPriority(isError ? TextEditor::TextMark::HighPriority : TextEditor::TextMark::NormalPriority);
    QIcon markIcon = diagnostic.icon();
    setIcon(markIcon.isNull() ? Icons::CODEMODEL_WARNING.icon() : markIcon);
    setToolTip(createDiagnosticToolTipString(diagnostic, std::nullopt, true));
    setLineAnnotation(diagnostic.description);
    setActionsProvider([diagnostic] {
        // Copy to clipboard action
        QList<QAction *> actions;
        QAction *action = new QAction();
        action->setIcon(Icon::fromTheme("edit-copy"));
        action->setToolTip(Tr::tr("Copy to Clipboard"));
        QObject::connect(action, &QAction::triggered, [diagnostic] {
            const QString text = createFullLocationString(diagnostic.location)
                                 + ": "
                                 + diagnostic.description;
            setClipboardAndSelection(text);
        });
        actions << action;

        // Disable diagnostic action
        action = new QAction();
        action->setIcon(Icons::BROKEN.icon());
        action->setToolTip(Tr::tr("Disable Diagnostic"));
        QObject::connect(action, &QAction::triggered, [diagnostic] { disableChecks({diagnostic}); });
        actions << action;
        return actions;
    });
}

DiagnosticMark::DiagnosticMark(const Diagnostic &diagnostic)
    : DiagnosticMark(diagnostic, TextDocument::textDocumentForFilePath(diagnostic.location.filePath))
{}

void DiagnosticMark::disable()
{
    if (!m_enabled)
        return;
    m_enabled = false;
    if (m_diagnostic.type == "error" || m_diagnostic.type == "fatal")
        setIcon(Icons::CODEMODEL_DISABLED_ERROR.icon());
    else
        setIcon(Icons::CODEMODEL_DISABLED_WARNING.icon());
    setColor(Theme::Color::IconsDisabledColor);
}

bool DiagnosticMark::enabled() const
{
    return m_enabled;
}

Diagnostic DiagnosticMark::diagnostic() const
{
    return m_diagnostic;
}

} // namespace Internal
} // namespace ClangTools

