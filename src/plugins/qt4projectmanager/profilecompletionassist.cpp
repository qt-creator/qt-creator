/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "profilecompletionassist.h"
#include "qt4projectmanagerconstants.h"
#include "profilekeywords.h"

#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/completionsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/basetexteditor.h>

#include <cplusplus/Icons.h>

#include <QTextCursor>

using namespace Qt4ProjectManager::Internal;
using namespace TextEditor;

// -------------------------
// ProFileAssistProposalItem
// -------------------------
ProFileAssistProposalItem::ProFileAssistProposalItem()
{}

ProFileAssistProposalItem::~ProFileAssistProposalItem()
{}

bool ProFileAssistProposalItem::prematurelyApplies(const QChar &c) const
{
    // only '(' in case of a function
    if (c == QLatin1Char('(') && ProFileKeywords::isFunction(text()))
        return true;
    return false;
}

void ProFileAssistProposalItem::applyContextualContent(TextEditor::BaseTextEditor *editor,
                                                        int basePosition) const
{
    const CompletionSettings &settings = TextEditorSettings::instance()->completionSettings();

    int replaceLength = editor->position() - basePosition;
    QString toInsert = text();
    int cursorOffset = 0;
    if (ProFileKeywords::isFunction(toInsert) && settings.m_autoInsertBrackets) {
        if (settings.m_spaceAfterFunctionName) {
            if (editor->textAt(editor->position(), 2) == QLatin1String(" (")) {
                cursorOffset = 2;
            } else if (editor->characterAt(editor->position()) == QLatin1Char('(')
                       || editor->characterAt(editor->position()) == QLatin1Char(' ')) {
                replaceLength += 1;
                toInsert += QLatin1String(" (");
            } else {
                toInsert += QLatin1String(" ()");
                cursorOffset = -1;
            }
        } else {
            if (editor->characterAt(editor->position()) == QLatin1Char('(')) {
                cursorOffset = 1;
            } else {
                toInsert += QLatin1String("()");
                cursorOffset = -1;
            }
        }
    }

    editor->setCursorPosition(basePosition);
    editor->replace(replaceLength, toInsert);
    if (cursorOffset)
        editor->setCursorPosition(editor->position() + cursorOffset);
}

// -------------------------------
// ProFileCompletionAssistProvider
// -------------------------------
ProFileCompletionAssistProvider::ProFileCompletionAssistProvider()
{}

ProFileCompletionAssistProvider::~ProFileCompletionAssistProvider()
{}

bool ProFileCompletionAssistProvider::supportsEditor(const Core::Id &editorId) const
{
    return editorId == Qt4ProjectManager::Constants::PROFILE_EDITOR_ID;
}

bool ProFileCompletionAssistProvider::isAsynchronous() const
{
    return false;
}

int ProFileCompletionAssistProvider::activationCharSequenceLength() const
{
    return 0;
}

bool ProFileCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    Q_UNUSED(sequence);
    return false;
}

bool ProFileCompletionAssistProvider::isContinuationChar(const QChar &c) const
{
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

IAssistProcessor *ProFileCompletionAssistProvider::createProcessor() const
{
    return new ProFileCompletionAssistProcessor;
}

// --------------------------------
// ProFileCompletionAssistProcessor
// --------------------------------
ProFileCompletionAssistProcessor::ProFileCompletionAssistProcessor()
    : m_startPosition(-1)
    , m_variableIcon(CPlusPlus::Icons().iconForType(CPlusPlus::Icons::VarPublicIconType))
    , m_functionIcon(CPlusPlus::Icons().iconForType(CPlusPlus::Icons::FuncPublicIconType))
{}

ProFileCompletionAssistProcessor::~ProFileCompletionAssistProcessor()
{}

IAssistProposal *ProFileCompletionAssistProcessor::perform(const IAssistInterface *interface)
{
    m_interface.reset(interface);

    if (isInComment())
        return 0;

    if (interface->reason() == IdleEditor && !acceptsIdleEditor())
        return 0;

    if (m_startPosition == -1)
        m_startPosition = findStartOfName();

    QList<TextEditor::BasicProposalItem *> items;
    QStringList keywords = ProFileKeywords::variables() + ProFileKeywords::functions();
    for (int i = 0; i < keywords.count(); i++) {
        BasicProposalItem *item = new ProFileAssistProposalItem;
        item->setText(keywords[i]);
        item->setIcon(ProFileKeywords::isFunction(item->text()) ? m_functionIcon : m_variableIcon);
        items.append(item);
    }

    return new GenericProposal(m_startPosition, new ProFileAssistProposalModel(items));
}

bool ProFileCompletionAssistProcessor::acceptsIdleEditor()
{
    const int pos = m_interface->position();
    QChar characterUnderCursor = m_interface->characterAt(pos);
    if (!characterUnderCursor.isLetterOrNumber()) {
        m_startPosition = findStartOfName();
        if (pos - m_startPosition >= 3 && !isInComment())
            return true;
    }
    return false;
}

int ProFileCompletionAssistProcessor::findStartOfName(int pos) const
{
    if (pos == -1)
        pos = m_interface->position();
    QChar chr;

    // Skip to the start of a name
    do {
        chr = m_interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));

    return pos + 1;
}

bool ProFileCompletionAssistProcessor::isInComment() const
{
    QTextCursor tc(m_interface->textDocument());
    tc.setPosition(m_interface->position());
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    const QString &lineBeginning = tc.selectedText();
    if (lineBeginning.contains(QLatin1Char('#')))
        return true;
    return false;
}

// --------------------------
// ProFileAssistProposalModel
// --------------------------
ProFileAssistProposalModel::ProFileAssistProposalModel(
    const QList<BasicProposalItem *> &items)
    : BasicProposalItemListModel(items)
{}

ProFileAssistProposalModel::~ProFileAssistProposalModel()
{}

bool ProFileAssistProposalModel::isSortable(const QString &prefix) const
{
    Q_UNUSED(prefix)
    return false;
}
