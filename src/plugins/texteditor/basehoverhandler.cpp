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

BaseHoverHandler::BaseHoverHandler(QObject *parent) :
    QObject(parent), m_matchingHelpCandidate(-1)
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
    if (!ToolTip::instance()->isVisible() || m_matchingHelpCandidate == -1)
        process(editor, pos);

    if (m_matchingHelpCandidate != -1)
        editor->setContextHelpId(m_helpCandidates.at(m_matchingHelpCandidate).m_helpId);
    else
        editor->setContextHelpId(QString());
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

void BaseHoverHandler::reset()
{
    m_matchingHelpCandidate = -1;
    m_helpCandidates.clear();
    m_toolTip.clear();

    resetExtras();
}

void BaseHoverHandler::process(ITextEditor *editor, int pos)
{
    reset();
    identifyMatch(editor, pos);
    evaluateHelpCandidates();
    decorateToolTip(editor);
}

void BaseHoverHandler::resetExtras()
{}

void BaseHoverHandler::evaluateHelpCandidates()
{
    for (int i = 0; i < m_helpCandidates.size(); ++i) {
        if (helpIdExists(m_helpCandidates.at(i).m_helpId)) {
            m_matchingHelpCandidate = i;
            return;
        }
    }
}

void BaseHoverHandler::decorateToolTip(ITextEditor *)
{}

void BaseHoverHandler::operateTooltip(ITextEditor *editor, const QPoint &point)
{
    if (m_toolTip.isEmpty()) {
        TextEditor::ToolTip::instance()->hide();
    } else {
        if (m_matchingHelpCandidate != -1)
            addF1ToToolTip();

        ToolTip::instance()->show(point, TextContent(m_toolTip), editor->widget());
    }
}

bool BaseHoverHandler::helpIdExists(const QString &helpId) const
{
    if (!Core::HelpManager::instance()->linksForIdentifier(helpId).isEmpty())
        return true;
    return false;
}

void BaseHoverHandler::addHelpCandidate(const HelpCandidate &helpCandidate)
{ m_helpCandidates.append(helpCandidate); }

void BaseHoverHandler::setHelpCandidate(const HelpCandidate &helpCandidate, int index)
{ m_helpCandidates[index] = helpCandidate; }

const QList<BaseHoverHandler::HelpCandidate> &BaseHoverHandler::helpCandidates() const
{ return m_helpCandidates; }

const BaseHoverHandler::HelpCandidate &BaseHoverHandler::helpCandidate(int index) const
{ return m_helpCandidates.at(index); }

void BaseHoverHandler::setMatchingHelpCandidate(int index)
{ m_matchingHelpCandidate = index; }

int BaseHoverHandler::matchingHelpCandidate() const
{ return m_matchingHelpCandidate; }

QString BaseHoverHandler::getDocContents(const bool extended)
{
    Q_ASSERT(m_matchingHelpCandidate >= 0);

    return getDocContents(m_helpCandidates.at(m_matchingHelpCandidate), extended);
}

QString BaseHoverHandler::getDocContents(const HelpCandidate &help, const bool extended)
{
    if (extended)
        m_htmlDocExtractor.extractExtendedContents(1500, true);
    else
        m_htmlDocExtractor.extractFirstParagraphOnly();

    QString contents;
    QMap<QString, QUrl> helpLinks =
        Core::HelpManager::instance()->linksForIdentifier(help.m_helpId);
    foreach (const QUrl &url, helpLinks) {
        const QByteArray &html = Core::HelpManager::instance()->fileData(url);
        switch (help.m_category) {
        case HelpCandidate::Brief:
            contents = m_htmlDocExtractor.getClassOrNamespaceBrief(html, help.m_docMark);
            break;
        case HelpCandidate::ClassOrNamespace:
            contents = m_htmlDocExtractor.getClassOrNamespaceDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Function:
            contents = m_htmlDocExtractor.getFunctionDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Enum:
            contents = m_htmlDocExtractor.getEnumDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Typedef:
            contents = m_htmlDocExtractor.getTypedefDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Macro:
            contents = m_htmlDocExtractor.getMacroDescription(html, help.m_docMark);
            break;
        case HelpCandidate::QML:
            contents = m_htmlDocExtractor.getQMLItemDescription(html, help.m_docMark);
            break;

        default:
            break;
        }

        if (!contents.isEmpty())
            break;
    }
    return contents;
}

BaseTextEditor *BaseHoverHandler::baseTextEditor(ITextEditor *editor)
{
    if (!editor)
        return 0;
    return qobject_cast<BaseTextEditor *>(editor->widget());
}

bool BaseHoverHandler::extendToolTips(ITextEditor *editor)
{
    BaseTextEditor *baseEditor = baseTextEditor(editor);
    if (baseEditor && baseEditor->displaySettings().m_extendTooltips)
        return true;
    return false;
}
