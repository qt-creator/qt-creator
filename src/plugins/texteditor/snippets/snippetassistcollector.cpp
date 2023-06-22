// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippetassistcollector.h"
#include "snippetscollection.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/codeassist/assistproposalitem.h>

using namespace TextEditor;
using namespace Internal;

class SnippetProposalItem : public AssistProposalItemInterface
{
public:
    SnippetProposalItem(const Snippet &snippet, const QIcon &icon)
        : m_snippet(snippet)
        , m_icon(icon)
    {}

    QString text() const override
    {
        return m_snippet.trigger() + QLatin1Char(' ') + m_snippet.complement();
    }
    bool implicitlyApplies() const override { return false; }
    bool prematurelyApplies(const QChar &) const override { return false; }
    void apply(TextDocumentManipulatorInterface &manipulator, int basePosition) const override
    {
        manipulator.insertCodeSnippet(basePosition, m_snippet.content(), &Snippet::parse);
    }
    QIcon icon() const override { return m_icon; }
    QString detail() const override { return m_snippet.generateTip(); }
    bool isSnippet() const override { return true; }
    bool isValid() const override { return true; }
    quint64 hash() const override { return 0; }

private:
    const Snippet m_snippet;
    const QIcon m_icon;
};

static void appendSnippets(QList<AssistProposalItemInterface *> *items,
                    const QString &groupId,
                    const QIcon &icon,
                    int order)
{
    SnippetsCollection *collection = SnippetsCollection::instance();
    const int size = collection->totalActiveSnippets(groupId);
    for (int i = 0; i < size; ++i) {
        const Snippet &snippet = collection->snippet(i, groupId);
        auto item = new SnippetProposalItem(snippet, icon);
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
