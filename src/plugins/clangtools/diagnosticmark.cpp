// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diagnosticmark.h"

#include "clangtoolsconstants.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"

#include <utils/utilsicons.h>
#include <utils/stringutils.h>

#include <QAction>

namespace ClangTools {
namespace Internal {

DiagnosticMark::DiagnosticMark(const Diagnostic &diagnostic)
    : TextEditor::TextMark(diagnostic.location.filePath,
                           diagnostic.location.line,
                           Utils::Id(Constants::DIAGNOSTIC_MARK_ID))
    , m_diagnostic(diagnostic)
{
    setSettingsPage(Constants::SETTINGS_PAGE_ID);

    if (diagnostic.type == "error" || diagnostic.type == "fatal")
        setColor(Utils::Theme::CodeModel_Error_TextMarkColor);
    else
        setColor(Utils::Theme::CodeModel_Warning_TextMarkColor);
    setPriority(TextEditor::TextMark::HighPriority);
    QIcon markIcon = diagnostic.icon();
    setIcon(markIcon.isNull() ? Utils::Icons::CODEMODEL_WARNING.icon() : markIcon);
    setToolTip(createDiagnosticToolTipString(diagnostic, std::nullopt, true));
    setLineAnnotation(diagnostic.description);
    setActionsProvider([diagnostic] {
        // Copy to clipboard action
        QList<QAction *> actions;
        QAction *action = new QAction();
        action->setIcon(QIcon::fromTheme("edit-copy", Utils::Icons::COPY.icon()));
        action->setToolTip(tr("Copy to Clipboard"));
        QObject::connect(action, &QAction::triggered, [diagnostic] {
            const QString text = createFullLocationString(diagnostic.location)
                                 + ": "
                                 + diagnostic.description;
            Utils::setClipboardAndSelection(text);
        });
        actions << action;

        // Disable diagnostic action
        action = new QAction();
        action->setIcon(Utils::Icons::BROKEN.icon());
        action->setToolTip(tr("Disable Diagnostic"));
        QObject::connect(action, &QAction::triggered, [diagnostic] { disableChecks({diagnostic}); });
        actions << action;
        return actions;
    });
}

void DiagnosticMark::disable()
{
    if (!m_enabled)
        return;
    m_enabled = false;
    if (m_diagnostic.type == "error" || m_diagnostic.type == "fatal")
        setIcon(Utils::Icons::CODEMODEL_DISABLED_ERROR.icon());
    else
        setIcon(Utils::Icons::CODEMODEL_DISABLED_WARNING.icon());
    setColor(Utils::Theme::Color::IconsDisabledColor);
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

