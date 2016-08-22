/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "snippetassistcollector.h"
#include "snippetscollection.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/codeassist/assistproposalitem.h>

using namespace TextEditor;
using namespace Internal;

static void appendSnippets(QList<AssistProposalItemInterface *> *items,
                    const QString &groupId,
                    const QIcon &icon,
                    int order)
{
    SnippetsCollection *collection = SnippetsCollection::instance();
    const int size = collection->totalActiveSnippets(groupId);
    for (int i = 0; i < size; ++i) {
        const Snippet &snippet = collection->snippet(i, groupId);
        AssistProposalItem *item = new AssistProposalItem;
        item->setText(snippet.trigger() + QLatin1Char(' ') + snippet.complement());
        item->setData(snippet.content());
        item->setDetail(snippet.generateTip());
        item->setIcon(icon);
        item->setOrder(order);
        items->append(item);
    }
}


SnippetAssistCollector::SnippetAssistCollector(const QString &groupId, const QIcon &icon, int order)
    : m_groupId(groupId)
    , m_icon(icon)
    , m_order(order)
{}

void SnippetAssistCollector::setGroupId(const QString &gid)
{
    m_groupId = gid;
}

QString SnippetAssistCollector::groupId() const
{
    return m_groupId;
}

QList<AssistProposalItemInterface *> SnippetAssistCollector::collect() const
{
    QList<AssistProposalItemInterface *> snippets;
    if (m_groupId.isEmpty())
        return snippets;
    appendSnippets(&snippets, m_groupId, m_icon, m_order);
    if (m_groupId != Constants::TEXT_SNIPPET_GROUP_ID)
        appendSnippets(&snippets, Constants::TEXT_SNIPPET_GROUP_ID, m_icon, m_order);
    return snippets;
}
