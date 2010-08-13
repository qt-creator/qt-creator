/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cpphoverhandler.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextCursor>

using namespace CppEditor::Internal;
using namespace Core;

namespace {
    QString removeClassNameQualification(const QString &name) {
        const int index = name.lastIndexOf(QLatin1Char(':'));
        if (index == -1)
            return name;
        else
            return name.right(name.length() - index - 1);
    }
}

CppHoverHandler::CppHoverHandler(QObject *parent) : BaseHoverHandler(parent)
{}

CppHoverHandler::~CppHoverHandler()
{}

bool CppHoverHandler::acceptEditor(IEditor *editor)
{
    CPPEditorEditable *cppEditor = qobject_cast<CPPEditorEditable *>(editor);
    if (cppEditor)
        return true;
    return false;
}

void CppHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    CPPEditor *cppEditor = qobject_cast<CPPEditor *>(editor->widget());
    if (!cppEditor)
        return;

    if (!cppEditor->extraSelectionTooltip(pos).isEmpty()) {
        setToolTip(cppEditor->extraSelectionTooltip(pos));
    } else {
        QTextCursor tc(cppEditor->document());
        tc.setPosition(pos);

        CppElementEvaluator evaluator(cppEditor);
        evaluator.setTextCursor(tc);
        QSharedPointer<CppElement> cppElement = evaluator.identifyCppElement();
        if (!cppElement.isNull()) {
            setToolTip(cppElement->tooltip());
            foreach (const QString &helpId, cppElement->helpIdCandidates())
                addHelpCandidate(HelpCandidate(helpId,
                                               cppElement->helpMark(),
                                               cppElement->helpCategory()));
        }
    }
}

void CppHoverHandler::evaluateHelpCandidates()
{
    for (int i = 0; i < helpCandidates().size() && matchingHelpCandidate() == -1; ++i) {
        if (helpIdExists(helpCandidates().at(i).m_helpId)) {
            setMatchingHelpCandidate(i);
        } else {
            // There are class help ids with and without qualification.
            HelpCandidate candidate = helpCandidates().at(i);
            const QString &helpId = removeClassNameQualification(candidate.m_helpId);
            if (helpIdExists(helpId)) {
                candidate.m_helpId = helpId;
                setHelpCandidate(candidate, i);
                setMatchingHelpCandidate(i);
            }
        }
    }
}

void CppHoverHandler::decorateToolTip(TextEditor::ITextEditor *editor)
{
    if (matchingHelpCandidate() != -1) {
        const QString &contents = getDocContents(extendToolTips(editor));
        if (!contents.isEmpty()) {
            HelpCandidate::Category cat = helpCandidate(matchingHelpCandidate()).m_category;
            if (cat == HelpCandidate::ClassOrNamespace)
                appendToolTip(contents);
            else
                setToolTip(contents);
        } else {
            QString tip = Qt::escape(toolTip());
            tip.prepend(QLatin1String("<nobr>"));
            tip.append(QLatin1String("</nobr>"));
            setToolTip(tip);
        }
    }
}
