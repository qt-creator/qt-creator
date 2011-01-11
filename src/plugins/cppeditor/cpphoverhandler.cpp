/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cpphoverhandler.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <cplusplus/ModelManagerInterface.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/helpitem.h>

#include <QtGui/QTextCursor>
#include <QtCore/QUrl>

using namespace CppEditor::Internal;
using namespace Core;

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
            foreach (const QString &helpId, cppElement->helpIdCandidates()) {
                if (!Core::HelpManager::instance()->linksForIdentifier(helpId).isEmpty()) {
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
