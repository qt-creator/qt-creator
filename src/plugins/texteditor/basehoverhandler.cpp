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

    ICore *core = ICore::instance();
    const int dbgContext =
        core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);
    if (core->hasContext(dbgContext))
        return;

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
    decorateToolTip(editor);
}

void BaseHoverHandler::decorateToolTip(ITextEditor *editor)
{
    BaseTextEditor *baseEditor = baseTextEditor(editor);
    if (!baseEditor)
        return;

    if (lastHelpItemIdentified().isValid()) {
        const QString &contents = lastHelpItemIdentified().extractContent(false);
        if (!contents.isEmpty()) {
            appendToolTip(contents);
        } else {
            QString tip = Qt::escape(toolTip());
            tip.prepend(QLatin1String("<nobr>"));
            tip.append(QLatin1String("</nobr>"));
            setToolTip(tip);
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
