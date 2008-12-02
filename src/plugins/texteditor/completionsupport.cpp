/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "completionsupport.h"
#include "completionwidget.h"
#include "icompletioncollector.h"

#include <coreplugin/icore.h>
#include <texteditor/itexteditable.h>

#include <QString>
#include <QList>

using namespace TextEditor;
using namespace TextEditor::Internal;


CompletionSupport *CompletionSupport::instance(Core::ICore *core)
{
    static CompletionSupport *m_instance = 0;
    if (!m_instance) {
        m_instance = new CompletionSupport(core);
    }
    return m_instance;
}

CompletionSupport::CompletionSupport(Core::ICore *core)
    : QObject(core),
      m_completionList(0),
      m_startPosition(0),
      m_checkCompletionTrigger(false),
      m_editor(0)
{
    m_completionCollector = core->pluginManager()->getObject<ICompletionCollector>();
}

void CompletionSupport::performCompletion(const CompletionItem &item)
{
    item.m_collector->complete(item);
    m_checkCompletionTrigger = true;
}

void CompletionSupport::cleanupCompletions()
{
    if (m_completionList)
        disconnect(m_completionList, SIGNAL(destroyed(QObject*)),
                   this, SLOT(cleanupCompletions()));

    m_completionList = 0;
    m_completionCollector->cleanup();

    if (m_checkCompletionTrigger) {
        m_checkCompletionTrigger = false;

        // Only check for completion trigger when some text was entered
        if (m_editor->position() > m_startPosition)
            autoComplete(m_editor, false);
    }
}

void CompletionSupport::autoComplete(ITextEditable *editor, bool forced)
{
    if (!m_completionCollector)
        return;

    m_editor = editor;
    QList<CompletionItem> completionItems;

    if (!m_completionList) {
        if (!forced && !m_completionCollector->triggersCompletion(editor))
            return;

        m_startPosition = m_completionCollector->startCompletion(editor);
        completionItems = getCompletions();

        Q_ASSERT(m_startPosition != -1 || completionItems.size() == 0);

        if (completionItems.isEmpty()) {
            cleanupCompletions();
            return;
        }

        m_completionList = new CompletionWidget(this, editor);

        connect(m_completionList, SIGNAL(itemSelected(TextEditor::CompletionItem)),
                this, SLOT(performCompletion(TextEditor::CompletionItem)));
        connect(m_completionList, SIGNAL(completionListClosed()),
                this, SLOT(cleanupCompletions()));

        // Make sure to clean up the completions if the list is destroyed without
        // emitting completionListClosed (can happen when no focus out event is received,
        // for example when switching applications on the Mac)
        connect(m_completionList, SIGNAL(destroyed(QObject*)),
                this, SLOT(cleanupCompletions()));
    } else {
        completionItems = getCompletions();

        if (completionItems.isEmpty()) {
            m_completionList->closeList();
            return;
        }
    }

    m_completionList->setCompletionItems(completionItems);

    // Partially complete when completion was forced
    if (forced && m_completionCollector->partiallyComplete(completionItems)) {
        m_checkCompletionTrigger = true;
        m_completionList->closeList();
    } else {
        m_completionList->showCompletions(m_startPosition);
    }
}

static bool completionItemLessThan(const CompletionItem &i1, const CompletionItem &i2)
{
    // The order is case-insensitive in principle, but case-sensitive when this would otherwise mean equality
    const int c = i1.m_text.compare(i2.m_text, Qt::CaseInsensitive);
    return c ? c < 0 : i1.m_text < i2.m_text;
}

QList<CompletionItem> CompletionSupport::getCompletions() const
{
    QList<CompletionItem> completionItems;

    m_completionCollector->completions(&completionItems);

    qStableSort(completionItems.begin(), completionItems.end(), completionItemLessThan);

    // Remove duplicates
    QString lastKey;
    QList<CompletionItem> uniquelist;

    foreach (const CompletionItem item, completionItems) {
        if (item.m_text != lastKey) {
            uniquelist.append(item);
            lastKey = item.m_text;
        }
    }

    return uniquelist;
}
