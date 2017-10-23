/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cpphoverhandler.h"

#include "cppeditorconstants.h"
#include "cppelementevaluator.h"

#include <coreplugin/helpmanager.h>
#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/editordocumenthandle.h>
#include <texteditor/texteditor.h>

#include <utils/textutils.h>
#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>

#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

using namespace Core;
using namespace TextEditor;

namespace {

CppTools::BaseEditorDocumentProcessor *editorDocumentProcessor(TextEditorWidget *editorWidget)
{
    const QString filePath = editorWidget->textDocument()->filePath().toString();
    auto cppModelManager = CppTools::CppModelManager::instance();
    CppTools::CppEditorDocumentHandle *editorHandle = cppModelManager->cppEditorDocument(filePath);

    if (editorHandle)
        return editorHandle->processor();

    return 0;
}

bool editorDocumentProcessorHasDiagnosticAt(TextEditorWidget *editorWidget, int pos)
{
    if (CppTools::BaseEditorDocumentProcessor *processor = editorDocumentProcessor(editorWidget)) {
        int line, column;
        if (Utils::Text::convertPosition(editorWidget->document(), pos, &line, &column))
            return processor->hasDiagnosticsAt(line, column);
    }

    return false;
}

void processWithEditorDocumentProcessor(TextEditorWidget *editorWidget,
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

} // anonymous namespace

namespace CppEditor {
namespace Internal {

void CppHoverHandler::identifyMatch(TextEditorWidget *editorWidget, int pos)
{
    m_positionForEditorDocumentProcessor = -1;

    if (editorDocumentProcessorHasDiagnosticAt(editorWidget, pos)) {
        setPriority(Priority_Diagnostic);
        m_positionForEditorDocumentProcessor = pos;
        return;
    }

    QTextCursor tc(editorWidget->document());
    tc.setPosition(pos);

    CppElementEvaluator evaluator(editorWidget);
    evaluator.setTextCursor(tc);
    evaluator.execute();
    if (evaluator.hasDiagnosis()) {
        setToolTip(evaluator.diagnosis());
        setPriority(Priority_Diagnostic);
    } else if (evaluator.identifiedCppElement()) {
        const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
        if (priority() != Priority_Diagnostic) {
            setToolTip(cppElement->tooltip);
            setPriority(cppElement->tooltip.isEmpty() ? Priority_None : Priority_Tooltip);
        }
        QStringList candidates = cppElement->helpIdCandidates;
        candidates.removeDuplicates();
        foreach (const QString &helpId, candidates) {
            if (helpId.isEmpty())
                continue;

            const QMap<QString, QUrl> helpLinks = HelpManager::linksForIdentifier(helpId);
            if (!helpLinks.isEmpty()) {
                setLastHelpItemIdentified(HelpItem(helpId,
                                                   cppElement->helpMark,
                                                   cppElement->helpCategory,
                                                   helpLinks));
                break;
            }
        }
    }
}

void CppHoverHandler::decorateToolTip()
{
    if (m_positionForEditorDocumentProcessor != -1)
        return;

    if (Qt::mightBeRichText(toolTip()))
        setToolTip(toolTip().toHtmlEscaped());

    if (priority() != Priority_Diagnostic)
        return;

    const HelpItem &help = lastHelpItemIdentified();
    if (help.isValid()) {
        // If Qt is built with a namespace, we still show the tip without it, as
        // it is in the docs and for consistency with the doc extraction mechanism.
        const HelpItem::Category category = help.category();
        const QString &contents = help.extractContent(false);
        if (!contents.isEmpty()) {
            if (category == HelpItem::ClassOrNamespace)
                setToolTip(help.helpId() + contents);
            else
                setToolTip(contents);
        } else if (category == HelpItem::Typedef ||
                   category == HelpItem::Enum ||
                   category == HelpItem::ClassOrNamespace) {
            // This approach is a bit limited since it cannot be used for functions
            // because the help id doesn't really help in that case.
            QString prefix;
            if (category == HelpItem::Typedef)
                prefix = QLatin1String("typedef ");
            else if (category == HelpItem::Enum)
                prefix = QLatin1String("enum ");
            setToolTip(prefix + help.helpId());
        }
    }
}

void CppHoverHandler::operateTooltip(TextEditor::TextEditorWidget *editorWidget,
                                     const QPoint &point)
{
    if (m_positionForEditorDocumentProcessor == -1) {
        BaseHoverHandler::operateTooltip(editorWidget, point);
        return;
    }

    const HelpItem helpItem = lastHelpItemIdentified();
    const QString helpId = helpItem.isValid() ? helpItem.helpId() : QString();
    processWithEditorDocumentProcessor(editorWidget, point, m_positionForEditorDocumentProcessor,
                                       helpId);
}

} // namespace Internal
} // namespace CppEditor
