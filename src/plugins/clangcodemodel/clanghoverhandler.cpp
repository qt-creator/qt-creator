/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clanghoverhandler.h"

#include "clangeditordocumentprocessor.h"

#include <coreplugin/helpmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/editordocumenthandle.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>
#include <utils/textutils.h>
#include <utils/tooltip/tooltip.h>

#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTextCodec>
#include <QVBoxLayout>

Q_LOGGING_CATEGORY(hoverLog, "qtc.clangcodemodel.hover", QtWarningMsg);

using namespace TextEditor;

namespace ClangCodeModel {
namespace Internal {

static CppTools::BaseEditorDocumentProcessor *editorDocumentProcessor(TextEditorWidget *editorWidget)
{
    const QString filePath = editorWidget->textDocument()->filePath().toString();
    auto cppModelManager = CppTools::CppModelManager::instance();
    CppTools::CppEditorDocumentHandle *editorHandle = cppModelManager->cppEditorDocument(filePath);

    if (editorHandle)
        return editorHandle->processor();

    return nullptr;
}

static TextMarks diagnosticTextMarksAt(TextEditorWidget *editorWidget, int position)
{
    const auto processor = qobject_cast<ClangEditorDocumentProcessor *>(
        editorDocumentProcessor(editorWidget));
    QTC_ASSERT(processor, return TextMarks());

    int line, column;
    const bool ok = Utils::Text::convertPosition(editorWidget->document(), position, &line, &column);
    QTC_ASSERT(ok, return TextMarks());

    return processor->diagnosticTextMarksAt(line, column);
}

static QFuture<CppTools::ToolTipInfo> editorDocumentHandlesToolTipInfo(
    TextEditorWidget *editorWidget, int pos)
{
    const QByteArray textCodecName = editorWidget->textDocument()->codec()->name();
    if (CppTools::BaseEditorDocumentProcessor *processor = editorDocumentProcessor(editorWidget)) {
        int line, column;
        if (Utils::Text::convertPosition(editorWidget->document(), pos, &line, &column))
            return processor->toolTipInfo(textCodecName, line, column + 1);
    }

    return QFuture<CppTools::ToolTipInfo>();
}

ClangHoverHandler::~ClangHoverHandler()
{
    abort();
}

void ClangHoverHandler::identifyMatch(TextEditorWidget *editorWidget,
                                      int pos,
                                      BaseHoverHandler::ReportPriority report)
{
    // Reset
    m_futureWatcher.reset();
    m_cursorPosition = -1;

    // Check for diagnostics (sync)
    if (!isContextHelpRequest() && !diagnosticTextMarksAt(editorWidget, pos).isEmpty()) {
        qCDebug(hoverLog) << "Checking for diagnostic at" << pos;
        setPriority(Priority_Diagnostic);
        m_cursorPosition = pos;
    }

    // Check for tooltips (async)
    QFuture<CppTools::ToolTipInfo> future = editorDocumentHandlesToolTipInfo(editorWidget, pos);
    if (QTC_GUARD(future.isRunning())) {
        qCDebug(hoverLog) << "Requesting tooltip info at" << pos;
        m_reportPriority = report;
        m_futureWatcher.reset(new QFutureWatcher<CppTools::ToolTipInfo>());
        QTextCursor tc(editorWidget->document());
        tc.setPosition(pos);
        const QStringList fallback = CppTools::identifierWordsUnderCursor(tc);
        QObject::connect(m_futureWatcher.data(),
                         &QFutureWatcherBase::finished,
                         [this, fallback]() {
                             if (m_futureWatcher->isCanceled()) {
                                 m_reportPriority(Priority_None);
                             } else {
                                 CppTools::ToolTipInfo info = m_futureWatcher->result();
                                 qCDebug(hoverLog)
                                     << "Appending word-based fallback lookup" << fallback;
                                 info.qDocIdCandidates += fallback;
                                 processToolTipInfo(info);
                             }
                         });
        m_futureWatcher->setFuture(future);
        return;
    }

    report(priority()); // Ops, something went wrong.
}

void ClangHoverHandler::abort()
{
    if (m_futureWatcher) {
        m_futureWatcher->cancel();
        m_futureWatcher.reset();
    }
}

#define RETURN_TEXT_FOR_CASE(enumValue) case Core::HelpItem::enumValue: return #enumValue
static const char *helpItemCategoryAsString(Core::HelpItem::Category category)
{
    switch (category) {
        RETURN_TEXT_FOR_CASE(Unknown);
        RETURN_TEXT_FOR_CASE(ClassOrNamespace);
        RETURN_TEXT_FOR_CASE(Enum);
        RETURN_TEXT_FOR_CASE(Typedef);
        RETURN_TEXT_FOR_CASE(Macro);
        RETURN_TEXT_FOR_CASE(Brief);
        RETURN_TEXT_FOR_CASE(Function);
        RETURN_TEXT_FOR_CASE(QmlComponent);
        RETURN_TEXT_FOR_CASE(QmlProperty);
        RETURN_TEXT_FOR_CASE(QMakeVariableOfFunction);
    }

    return "UnhandledHelpItemCategory";
}
#undef RETURN_TEXT_FOR_CASE

void ClangHoverHandler::processToolTipInfo(const CppTools::ToolTipInfo &info)
{
    qCDebug(hoverLog) << "Processing tooltip info" << info.text;

    QString text = info.text;
    if (!info.briefComment.isEmpty())
        text.append("\n\n" + info.briefComment);

    qCDebug(hoverLog) << "Querying help manager with"
                      << info.qDocIdCandidates
                      << info.qDocMark
                      << helpItemCategoryAsString(info.qDocCategory);
    setLastHelpItemIdentified({info.qDocIdCandidates, info.qDocMark, info.qDocCategory});

    if (!info.sizeInBytes.isEmpty())
        text.append("\n\n" + tr("%1 bytes").arg(info.sizeInBytes));

    setToolTip(text);
    m_reportPriority(priority());
}

void ClangHoverHandler::operateTooltip(TextEditor::TextEditorWidget *editorWidget,
                                       const QPoint &point)
{
    if (priority() == Priority_Diagnostic) {
        const TextMarks textMarks = diagnosticTextMarksAt(editorWidget, m_cursorPosition);
        editorWidget->showTextMarksToolTip(point, textMarks);
        return;
    }

    // Priority_Tooltip / Priority_Help
    BaseHoverHandler::operateTooltip(editorWidget, point);
}

} // namespace Internal
} // namespace ClangCodeModel
