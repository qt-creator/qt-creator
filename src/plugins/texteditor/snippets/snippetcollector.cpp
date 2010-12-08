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

#include "snippetcollector.h"
#include "snippetscollection.h"

#include <texteditor/texteditorconstants.h>

using namespace TextEditor;
using namespace Internal;

namespace {

void appendSnippets(ICompletionCollector *collector,
                    QList<CompletionItem> *completionItems,
                    const QString &groupId,
                    const QIcon &icon,
                    int order)
{
    SnippetsCollection *collection = SnippetsCollection::instance();
    const int size = collection->totalActiveSnippets(groupId);
    for (int i = 0; i < size; ++i) {
        const Snippet &snippet = collection->snippet(i, groupId);
        CompletionItem item(collector);
        item.text = snippet.trigger() + QLatin1Char(' ') + snippet.complement();
        item.data = snippet.content();
        item.details = snippet.generateTip();
        item.icon = icon;
        item.order = order;
        item.isSnippet = true;
        completionItems->append(item);
    }
}

} // anonymous

SnippetCollector::SnippetCollector(const QString &groupId, const QIcon &icon, int order) :
    m_groupId(groupId), m_icon(icon), m_order(order)
{}

SnippetCollector::~SnippetCollector()
{}

QList<CompletionItem> SnippetCollector::getSnippets(ICompletionCollector *collector) const
{
    QList<CompletionItem> completionItems;
    appendSnippets(collector, &completionItems, m_groupId, m_icon, m_order);
    appendSnippets(collector, &completionItems, Constants::TEXT_SNIPPET_GROUP_ID, m_icon, m_order);
    return completionItems;
}
