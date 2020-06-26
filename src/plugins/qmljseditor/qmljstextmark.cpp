/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

static Utils::Id cartegoryForSeverity(QmlJS::Severity::Enum kind)
{
    return isWarning(kind) ? QMLJS_WARNING : QMLJS_ERROR;
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
    setDefaultToolTip(warning ? QApplication::translate("QmlJS Code Model Marks", "Code Model Warning")
                              : QApplication::translate("QmlJS Code Model Marks", "Code Model Error"));
    setToolTip(message);
    setPriority(warning ? TextEditor::TextMark::NormalPriority
                        : TextEditor::TextMark::HighPriority);
    setLineAnnotation(message);
}
