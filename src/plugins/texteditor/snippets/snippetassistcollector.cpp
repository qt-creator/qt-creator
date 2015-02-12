/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "snippetassistcollector.h"
#include "snippetscollection.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/codeassist/assistproposalitem.h>

using namespace TextEditor;
using namespace Internal;

static void appendSnippets(QList<AssistProposalItem *> *items,
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

SnippetAssistCollector::~SnippetAssistCollector()
{}

QList<AssistProposalItem *> SnippetAssistCollector::collect() const
{
    QList<AssistProposalItem *> snippets;
    appendSnippets(&snippets, m_groupId, m_icon, m_order);
    appendSnippets(&snippets, QLatin1String(Constants::TEXT_SNIPPET_GROUP_ID), m_icon, m_order);
    return snippets;
}
