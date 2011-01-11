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

#include "basehoverhandler.h"
#include "itexteditor.h"
#include "basetexteditor.h"
#include "displaysettings.h"
#include "tooltip.h"
#include "tipcontents.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <debugger/debuggerconstants.h>

#include <QtCore/QPoint>

using namespace TextEditor;
using namespace Core;

BaseHoverHandler::BaseHoverHandler(QObject *parent) : QObject(parent)
{
    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(ICore::instance()->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(editorOpened(Core::IEditor *)));
}

BaseHoverHandler::~BaseHoverHandler()
{}

void BaseHoverHandler::editorOpened(Core::IEditor *editor)
{
    if (acceptEditor(editor)) {
        BaseTextEditorEditable *textEditor = qobject_cast<BaseTextEditorEditable *>(editor);
        if (textEditor) {
            connect(textEditor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*, QPoint, int)),
                    this, SLOT(showToolTip(TextEditor::ITextEditor*, QPoint, int)));

            connect(textEditor, SIGNAL(contextHelpIdRequested(TextEditor::ITextEditor*, int)),
                    this, SLOT(updateContextHelpId(TextEditor::ITextEditor*, int)));
        }
    }
}

void BaseHoverHandler::showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos)
{
    BaseTextEditor *baseEditor = baseTextEditor(editor);
    if (!baseEditor)
        return;

    editor->setContextHelpId(QString());

    process(editor, pos);

    const QPoint &actualPoint = point - QPoint(0,
#ifdef Q_WS_WIN
    24
#else
    16
#endif
    );

    operateTooltip(editor, actualPoint);
}

void BaseHoverHandler::updateContextHelpId(TextEditor::ITextEditor *editor, int pos)
{
    BaseTextEditor *baseEditor = baseTextEditor(editor);
    if (!baseEditor)
        return;

    // If the tooltip is visible and there is a help match, this match is used to update
    // the help id. Otherwise, let the identification process happen.
    if (!ToolTip::instance()->isVisible() || !lastHelpItemIdentified().isValid())
        process(editor, pos);

    if (lastHelpItemIdentified().isValid())
        editor->setContextHelpId(lastHelpItemIdentified().helpId());
    else
        editor->setContextHelpId(QString()); // Make sure it's an empty string.
}

void BaseHoverHandler::setToolTip(const QString &tooltip)
{ m_toolTip = tooltip; }

const QString &BaseHoverHandler::toolTip() const
{ return m_toolTip; }

void BaseHoverHandler::appendToolTip(const QString &extension)
{ m_toolTip.append(extension); }

void BaseHoverHandler::addF1ToToolTip()
{
    m_toolTip = QString(QLatin1String("<table><tr><td valign=middle>%1</td><td>&nbsp;&nbsp;"
                                      "<img src=\":/cppeditor/images/f1.png\"></td>"
                                      "</tr></table>")).arg(m_toolTip);
}

void BaseHoverHandler::setLastHelpItemIdentified(const HelpItem &help)
{ m_lastHelpItemIdentified = help; }

const HelpItem &BaseHoverHandler::lastHelpItemIdentified() const
{ return m_lastHelpItemIdentified; }

void BaseHoverHandler::clear()
{
    m_toolTip.clear();
    m_lastHelpItemIdentified = HelpItem();
}

void BaseHoverHandler::process(ITextEditor *editor, int pos)
{
    clear();
    identifyMatch(editor, pos);
    decorateToolTip();
}

void BaseHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(Qt::escape(toolTip()));

    if (lastHelpItemIdentified().isValid()) {
        const QString &contents = lastHelpItemIdentified().extractContent(false);
        if (!contents.isEmpty()) {
            setToolTip(Qt::escape(toolTip()));
            appendToolTip(contents);
        }
        addF1ToToolTip();
    }
}

void BaseHoverHandler::operateTooltip(ITextEditor *editor, const QPoint &point)
{
    if (m_toolTip.isEmpty())
        ToolTip::instance()->hide();
    else
        ToolTip::instance()->show(point, TextContent(m_toolTip), editor->widget());
}

BaseTextEditor *BaseHoverHandler::baseTextEditor(ITextEditor *editor)
{
    if (!editor)
        return 0;
    return qobject_cast<BaseTextEditor *>(editor->widget());
}
