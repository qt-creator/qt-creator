// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditortr.h"
#include "qmljstextmark.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QApplication>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace Utils;

using namespace TextEditor;

const char QMLJS_ERROR[] = "QmlJS.Error";
const char QMLJS_WARNING[] = "QmlJS.Warning";

static bool isWarning(QmlJS::Severity::Enum kind)
{
    switch (kind) {
    case QmlJS::Severity::Hint:
    case QmlJS::Severity::MaybeWarning:
    case QmlJS::Severity::Warning:
    case QmlJS::Severity::ReadingTypeInfoWarning:
        return true;
    case QmlJS::Severity::MaybeError:
    case QmlJS::Severity::Error:
        break;
    }
    return false;
}

static TextMarkCategory cartegoryForSeverity(QmlJS::Severity::Enum kind)
{
    return isWarning(kind) ? TextMarkCategory{"QML Warning", QMLJS_WARNING}
                           : TextMarkCategory{"QML Error", QMLJS_ERROR};
}

QmlJSTextMark::QmlJSTextMark(const FilePath &fileName,
                             const QmlJS::DiagnosticMessage &diagnostic,
                             const QmlJSTextMark::RemovedFromEditorHandler &removedHandler)
    : TextEditor::TextMark(fileName, int(diagnostic.loc.startLine),
                           cartegoryForSeverity(diagnostic.kind))
    , m_removedFromEditorHandler(removedHandler)

{
    init(isWarning(diagnostic.kind), diagnostic.message);
}

QmlJSTextMark::QmlJSTextMark(const FilePath &fileName,
                             const QmlJS::StaticAnalysis::Message &message,
                             const QmlJSTextMark::RemovedFromEditorHandler &removedHandler)
    : TextEditor::TextMark(fileName, int(message.location.startLine),
                           cartegoryForSeverity(message.severity))
    , m_removedFromEditorHandler(removedHandler)
{
    init(isWarning(message.severity), message.message);
}

void QmlJSTextMark::removedFromEditor()
{
    QTC_ASSERT(m_removedFromEditorHandler, return);
    m_removedFromEditorHandler(this);
}

void QmlJSTextMark::init(bool warning, const QString &message)
{
    setIcon(warning ? Utils::Icons::CODEMODEL_WARNING.icon()
                    : Utils::Icons::CODEMODEL_ERROR.icon());
    setColor(warning ? Utils::Theme::CodeModel_Warning_TextMarkColor
                     : Utils::Theme::CodeModel_Error_TextMarkColor);
    setDefaultToolTip(warning ? Tr::tr("Code Model Warning")
                              : Tr::tr("Code Model Error"));
    setToolTip(message);
    setPriority(warning ? TextEditor::TextMark::NormalPriority
                        : TextEditor::TextMark::HighPriority);
    setLineAnnotation(message);
}
