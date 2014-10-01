/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpphoverhandler.h"

#include "cppeditorconstants.h"
#include "cppelementevaluator.h"

#include <coreplugin/helpmanager.h>
#include <texteditor/basetexteditor.h>

#include <utils/qtcassert.h>

#include <QTextCursor>
#include <QUrl>

using namespace CppEditor::Internal;
using namespace Core;

CppHoverHandler::CppHoverHandler(QObject *parent) : BaseHoverHandler(parent)
{}

CppHoverHandler::~CppHoverHandler()
{}

bool CppHoverHandler::acceptEditor(IEditor *editor)
{
    return editor->document()->id() == CppEditor::Constants::CPPEDITOR_ID;
}

void CppHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    using namespace TextEditor;
    BaseTextEditorWidget *textEditor = qobject_cast<BaseTextEditorWidget *>(editor->widget());
    QTC_ASSERT(textEditor, return);

    if (!textEditor->extraSelectionTooltip(pos).isEmpty()) {
        setToolTip(textEditor->extraSelectionTooltip(pos));
    } else {
        QTextCursor tc(textEditor->document());
        tc.setPosition(pos);

        CppElementEvaluator evaluator(textEditor);
        evaluator.setTextCursor(tc);
        evaluator.execute();
        if (evaluator.hasDiagnosis()) {
            setToolTip(evaluator.diagnosis());
            setIsDiagnosticTooltip(true);
        }
        if (evaluator.identifiedCppElement()) {
            const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
            if (!isDiagnosticTooltip())
                setToolTip(cppElement->tooltip);
            QStringList candidates = cppElement->helpIdCandidates;
            candidates.removeDuplicates();
            foreach (const QString &helpId, candidates) {
                if (helpId.isEmpty())
                    continue;

                const QMap<QString, QUrl> helpLinks = Core::HelpManager::linksForIdentifier(helpId);
                if (!helpLinks.isEmpty()) {
                    setLastHelpItemIdentified(TextEditor::HelpItem(helpId,
                                                                   cppElement->helpMark,
                                                                   cppElement->helpCategory,
                                                                   helpLinks));
                    break;
                }
            }
        }
    }
}

void CppHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(Qt::escape(toolTip()));

    if (isDiagnosticTooltip())
        return;

    const TextEditor::HelpItem &help = lastHelpItemIdentified();
    if (help.isValid()) {
        // If Qt is built with a namespace, we still show the tip without it, as
        // it is in the docs and for consistency with the doc extraction mechanism.
        const TextEditor::HelpItem::Category category = help.category();
        const QString &contents = help.extractContent(false);
        if (!contents.isEmpty()) {
            if (category == TextEditor::HelpItem::ClassOrNamespace)
                setToolTip(help.helpId() + contents);
            else
                setToolTip(contents);
        } else if (category == TextEditor::HelpItem::Typedef ||
                   category == TextEditor::HelpItem::Enum ||
                   category == TextEditor::HelpItem::ClassOrNamespace) {
            // This approach is a bit limited since it cannot be used for functions
            // because the help id doesn't really help in that case.
            QString prefix;
            if (category == TextEditor::HelpItem::Typedef)
                prefix = QLatin1String("typedef ");
            else if (category == TextEditor::HelpItem::Enum)
                prefix = QLatin1String("enum ");
            setToolTip(prefix + help.helpId());
        }
        addF1ToToolTip();
    }
}
