/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "completionsupport.h"
#include "completionwidget.h"
#include "icompletioncollector.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditable.h>
#include <utils/qtcassert.h>

#include <QtCore/QString>
#include <QtCore/QList>

#include <algorithm>

using namespace TextEditor;
using namespace TextEditor::Internal;


CompletionSupport *CompletionSupport::instance()
{
    static CompletionSupport *m_instance = 0;
    if (!m_instance)
        m_instance = new CompletionSupport;
    return m_instance;
}

CompletionSupport::CompletionSupport()
    : QObject(Core::ICore::instance()),
      m_completionList(0),
      m_startPosition(0),
      m_checkCompletionTrigger(false),
      m_editor(0)
{
    m_completionCollector = ExtensionSystem::PluginManager::instance()
        ->getObject<ICompletionCollector>();
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

        QTC_ASSERT(m_startPosition != -1 || completionItems.size() == 0, return);

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

static bool compareChar(const QChar &l, const QChar &r)
{
    if (l == QLatin1Char('_'))
        return false;
    else if (r == QLatin1Char('_'))
        return true;
    else
        return l < r;
}

static bool lessThan(const QString &l, const QString &r)
{
    return std::lexicographical_compare(l.begin(), l.end(),
                                        r.begin(), r.end(),
                                        compareChar);
}

static bool completionItemLessThan(const CompletionItem &i1, const CompletionItem &i2)
{
    // The order is case-insensitive in principle, but case-sensitive when this would otherwise mean equality
    const QString lower1 = i1.m_text.toLower();
    const QString lower2 = i2.m_text.toLower();
    if (lower1 == lower2)
        return lessThan(i1.m_text, i2.m_text);
    else
        return lessThan(lower1, lower2);
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
