// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckconstants.h"
#include "cppcheckdiagnostic.h"
#include "cppchecktextmark.h"
#include "cppchecktr.h"

#include <texteditor/texteditortr.h>

#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QMap>

using namespace TextEditor;
using namespace Utils;

namespace Cppcheck::Internal {

struct Visual
{
    Visual(Utils::Theme::Color color, TextMark::Priority priority,
           const QIcon &icon)
        : color(color),
          priority(priority),
          icon(icon)
    {}
    Utils::Theme::Color color;
    TextMark::Priority priority;
    QIcon icon;
};

static Visual getVisual(Diagnostic::Severity type)
{
    using Color = Theme::Color;
    using Priority = TextMark::Priority;

    static const QMap<Diagnostic::Severity, Visual> visuals{
        {Diagnostic::Severity::Error, {Color::IconsErrorColor, Priority::HighPriority,
                        Icons::CRITICAL.icon()}},
        {Diagnostic::Severity::Warning, {Color::IconsWarningColor, Priority::NormalPriority,
                        Icons::WARNING.icon()}},
    };

    return visuals.value(type, {Color::IconsInfoColor, Priority::LowPriority, Icons::INFO.icon()});
}

CppcheckTextMark::CppcheckTextMark(const Diagnostic &diagnostic)
    : TextEditor::TextMark(diagnostic.fileName,
                           diagnostic.lineNumber,
                           {Tr::tr("Cppcheck"), Utils::Id(Constants::TEXTMARK_CATEGORY_ID)})
    , m_severity(diagnostic.severity)
    , m_checkId(diagnostic.checkId)
    , m_message(diagnostic.message)
{
    const Visual visual = getVisual(diagnostic.severity);
    setPriority(visual.priority);
    setColor(visual.color);
    setIcon(visual.icon);
    setToolTip(toolTipText(diagnostic.severityText));
    setLineAnnotation(diagnostic.message);
    setSettingsPage(Constants::OPTIONS_PAGE_ID);
    setActionsProvider([diagnostic] {
        // Copy to clipboard action
        QAction *action = new QAction;
        action->setIcon(Icon::fromTheme("edit-copy"));
        action->setToolTip(TextEditor::Tr::tr("Copy to Clipboard"));
        QObject::connect(action, &QAction::triggered, [diagnostic]() {
            const QString text = QString("%1:%2: %3")
                    .arg(diagnostic.fileName.toUserOutput())
                    .arg(diagnostic.lineNumber)
                    .arg(diagnostic.message);
            Utils::setClipboardAndSelection(text);
        });
        return QList<QAction *>{action};
    });
}

QString CppcheckTextMark::toolTipText(const QString &severityText) const
{
    return QString(
                "<table cellspacing='0' cellpadding='0' width='100%'>"
                "  <tr>"
                "    <td align='left'><b>Cppcheck</b></td>"
                "    <td align='right'>&nbsp;<font color='gray'>%1: %2</font></td>"
                "  </tr>"
                "  <tr>"
                "    <td colspan='2' align='left' style='padding-left:10px'>%3</td>"
                "  </tr>"
                "</table>").arg(m_checkId, severityText, m_message);
}

} // Cppcheck::Internal
