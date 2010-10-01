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
#include <coreplugin/helpmanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/helpitem.h>

#include <QtGui/QTextCursor>
#include <QtCore/QUrl>

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
            foreach (QString helpId, cppElement->helpIdCandidates()) {
                bool found = false;
                if (!Core::HelpManager::instance()->linksForIdentifier(helpId).isEmpty()) {
                    found = true;
                } else {
                    helpId = removeClassNameQualification(helpId);
                    if (!Core::HelpManager::instance()->linksForIdentifier(helpId).isEmpty())
                        found = true;
                }
                if (found) {
                    setLastHelpItemIdentified(TextEditor::HelpItem(helpId,
                                                                   cppElement->helpMark(),
                                                                   cppElement->helpCategory()));
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
