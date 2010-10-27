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

#include "snippetprovider.h"
#include "snippetsmanager.h"
#include "snippetscollection.h"

using namespace TextEditor;
using namespace Internal;

SnippetProvider::SnippetProvider(Snippet::Group group, const QIcon &icon, int order) :
    m_group(group), m_icon(icon), m_order(order)
{}

SnippetProvider::~SnippetProvider()
{}

QList<CompletionItem> SnippetProvider::getSnippets(ICompletionCollector *collector) const
{
    QList<CompletionItem> completionItems;
    QSharedPointer<SnippetsCollection> collection =
        SnippetsManager::instance()->snippetsCollection();
    const int size = collection->totalActiveSnippets(m_group);
    for (int i = 0; i < size; ++i) {
        const Snippet &snippet = collection->snippet(i, m_group);
        CompletionItem item(collector);
        item.text = snippet.trigger() + QLatin1Char(' ') + snippet.complement();
        item.data = snippet.content();
        item.details = snippet.generateTip();
        item.icon = m_icon;
        item.order = m_order;

        completionItems.append(item);
    }
    return completionItems;
}
