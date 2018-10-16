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

#include <coreplugin/helpmanager.h>
#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/editordocumenthandle.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>
#include <utils/textutils.h>
#include <utils/tooltip/tooltip.h>

#include <QFutureWatcher>
#include <QLoggingCategory>
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

    return 0;
}

static bool editorDocumentProcessorHasDiagnosticAt(TextEditorWidget *editorWidget, int pos)
{
    if (CppTools::BaseEditorDocumentProcessor *processor = editorDocumentProcessor(editorWidget)) {
        int line, column;
        if (Utils::Text::convertPosition(editorWidget->document(), pos, &line, &column))
            return processor->hasDiagnosticsAt(line, column);
    }

    return false;
}

static void processWithEditorDocumentProcessor(TextEditorWidget *editorWidget,
                                               const QPoint &point,
                                               int position,
                                               const QString &helpId)
{
    if (CppTools::BaseEditorDocumentProcessor *processor = editorDocumentProcessor(editorWidget)) {
        int line, column;
        if (Utils::Text::convertPosition(editorWidget->document(), position, &line, &column)) {
            auto layout = new QVBoxLayout;
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(2);
            processor->addDiagnosticToolTipToLayout(line, column, layout);
            Utils::ToolTip::show(point, layout, editorWidget, helpId);
        }
    }
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
    if (editorDocumentProcessorHasDiagnosticAt(editorWidget, pos)) {
        qCDebug(hoverLog) << "Checking for diagnostic at" << pos;
        setPriority(Priority_Diagnostic);
        m_cursorPosition = pos;
        report(priority());
        return;
    }

    // Check for tooltips (async)
    QFuture<CppTools::ToolTipInfo> future = editorDocumentHandlesToolTipInfo(editorWidget, pos);
    if (QTC_GUARD(future.isRunning())) {
        qCDebug(hoverLog) << "Requesting tooltip info at" << pos;
        m_reportPriority = report;
        m_futureWatcher.reset(new QFutureWatcher<CppTools::ToolTipInfo>());
        QObject::connect(m_futureWatcher.data(), &QFutureWatcherBase::finished, [this]() {
            if (m_futureWatcher->isCanceled())
                m_reportPriority(Priority_None);
            else
                processToolTipInfo(m_futureWatcher->result());
        });
        m_futureWatcher->setFuture(future);
        return;
    }

    report(Priority_None); // Ops, something went wrong.
}

void ClangHoverHandler::abort()
{
    if (m_futureWatcher) {
        m_futureWatcher->cancel();
        m_futureWatcher.reset();
    }
}

#define RETURN_TEXT_FOR_CASE(enumValue) case TextEditor::HelpItem::enumValue: return #enumValue
static const char *helpItemCategoryAsString(TextEditor::HelpItem::Category category)
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

    for (const QString &qdocIdCandidate : info.qDocIdCandidates) {
        qCDebug(hoverLog) << "Querying help manager with"
                          << qdocIdCandidate
                          << info.qDocMark
                          << helpItemCategoryAsString(info.qDocCategory);
        const QMap<QString, QUrl> helpLinks = Core::HelpManager::linksForIdentifier(qdocIdCandidate);
        if (!helpLinks.isEmpty()) {
            qCDebug(hoverLog) << "  Match!";
            setLastHelpItemIdentified(
                HelpItem(qdocIdCandidate, info.qDocMark, info.qDocCategory, helpLinks));
            break;
        }
    }

    if (!info.sizeInBytes.isEmpty())
        text.append("\n\n" + tr("%1 bytes").arg(info.sizeInBytes));

    setToolTip(text);
    m_reportPriority(priority());
}

void ClangHoverHandler::decorateToolTip()
{
    if (priority() == Priority_Diagnostic)
        return;

    if (Qt::mightBeRichText(toolTip()))
        setToolTip(toolTip().toHtmlEscaped());

    const HelpItem &help = lastHelpItemIdentified();
    if (help.isValid()) {
        const QString text = CppTools::CppHoverHandler::tooltipTextForHelpItem(help);
        if (!text.isEmpty())
            setToolTip(text);
    }
}

void ClangHoverHandler::operateTooltip(TextEditor::TextEditorWidget *editorWidget,
                                       const QPoint &point)
{
    if (priority() == Priority_Diagnostic) {
        const HelpItem helpItem = lastHelpItemIdentified();
        const QString helpId = helpItem.isValid() ? helpItem.helpId() : QString();
        processWithEditorDocumentProcessor(editorWidget, point, m_cursorPosition, helpId);
        return;
    }

    // Priority_Tooltip / Priority_Help
    BaseHoverHandler::operateTooltip(editorWidget, point);
}

} // namespace Internal
} // namespace ClangCodeModel
