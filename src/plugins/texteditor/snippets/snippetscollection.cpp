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

#include "snippetscollection.h"

#include <QtAlgorithms>

#include <iterator>
#include <algorithm>

using namespace TextEditor;
using namespace Internal;

namespace {

struct SnippetComp
{
    bool operator()(const Snippet &a, const Snippet &b) const
    {
        const int comp = a.trigger().toLower().localeAwareCompare(b.trigger().toLower());
        if (comp < 0)
            return true;
        else if (comp == 0 &&
                 a.complement().toLower().localeAwareCompare(b.complement().toLower()) < 0)
            return true;
        return false;
    }
};
SnippetComp snippetComp;

struct RemovedSnippetPred
{
    bool operator()(const Snippet &s) const
    {
        return s.isRemoved();
    }
};
RemovedSnippetPred removedSnippetPred;

} // Anonymous

// Hint
SnippetsCollection::Hint::Hint(int index) : m_index(index)
{}

SnippetsCollection::Hint::Hint(int index, QList<Snippet>::iterator it) : m_index(index), m_it(it)
{}

int SnippetsCollection::Hint::index() const
{
    return m_index;
}

// SnippetsCollection
SnippetsCollection::SnippetsCollection() :
    m_snippets(Snippet::GroupSize),
    m_activeSnippetsEnd(Snippet::GroupSize)
{
    for (Snippet::Group group = Snippet::Cpp; group < Snippet::GroupSize; ++group)
        m_activeSnippetsEnd[group] = m_snippets[group].end();
}

SnippetsCollection::~SnippetsCollection()
{}

void SnippetsCollection::insertSnippet(const Snippet &snippet, Snippet::Group group)
{
    insertSnippet(snippet, group, computeInsertionHint(snippet, group));
}

void SnippetsCollection::insertSnippet(const Snippet &snippet,
                                       Snippet::Group group,
                                       const Hint &hint)
{
    if (snippet.isBuiltIn() && snippet.isRemoved()) {
        m_activeSnippetsEnd[group] = m_snippets[group].insert(m_activeSnippetsEnd[group], snippet);
    } else {
        m_snippets[group].insert(hint.m_it, snippet);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeInsertionHint(const Snippet &snippet,
                                                                  Snippet::Group group)
{
    QList<Snippet> &snippets = m_snippets[group];
    QList<Snippet>::iterator it = qUpperBound(
        snippets.begin(), m_activeSnippetsEnd.at(group), snippet, snippetComp);
    return Hint(static_cast<int>(std::distance(snippets.begin(), it)), it);
}

void SnippetsCollection::replaceSnippet(int index, const Snippet &snippet, Snippet::Group group)
{
    replaceSnippet(index, snippet, group, computeReplacementHint(index, snippet, group));
}

void SnippetsCollection::replaceSnippet(int index,
                                        const Snippet &snippet,
                                        Snippet::Group group,
                                        const Hint &hint)
{
    Snippet replacement(snippet);
    if (replacement.isBuiltIn() && !replacement.isModified())
        replacement.setIsModified(true);

    if (index == hint.index()) {
        m_snippets[group][index] = replacement;
    } else {
        insertSnippet(replacement, group, hint);
        // Consider whether the row moved up towards the beginning or down towards the end.
        if (index < hint.index())
            m_snippets[group].removeAt(index);
        else
            m_snippets[group].removeAt(index + 1);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeReplacementHint(int index,
                                                                    const Snippet &snippet,
                                                                    Snippet::Group group)
{
    QList<Snippet> &snippets = m_snippets[group];
    QList<Snippet>::iterator it = qLowerBound(
        snippets.begin(), m_activeSnippetsEnd.at(group), snippet, snippetComp);
    int hintIndex = static_cast<int>(std::distance(snippets.begin(), it));
    if (index < hintIndex - 1)
        return Hint(hintIndex - 1, it);
    it = qUpperBound(it, m_activeSnippetsEnd.at(group), snippet, snippetComp);
    hintIndex = static_cast<int>(std::distance(snippets.begin(), it));
    if (index > hintIndex)
        return Hint(hintIndex, it);
    // Even if the snipet is at a different index it is still inside a valid range.
    return Hint(index);
}

void SnippetsCollection::removeSnippet(int index, Snippet::Group group)
{
    Snippet snippet(m_snippets.at(group).at(index));
    m_snippets[group].removeAt(index);
    if (snippet.isBuiltIn()) {
        snippet.setIsRemoved(true);
        m_activeSnippetsEnd[group] = m_snippets[group].insert(m_activeSnippetsEnd[group], snippet);
    } else {
        updateActiveSnippetsEnd(group);
    }
}

const Snippet &SnippetsCollection::snippet(int index, Snippet::Group group) const
{
    return m_snippets.at(group).at(index);
}

void SnippetsCollection::setSnippetContent(int index, Snippet::Group group, const QString &content)
{
    Snippet &snippet = m_snippets[group][index];
    snippet.setContent(content);
    if (snippet.isBuiltIn() && !snippet.isModified())
        snippet.setIsModified(true);
}

int SnippetsCollection::totalActiveSnippets(Snippet::Group group) const
{
    return std::distance<QList<Snippet>::const_iterator>(m_snippets.at(group).begin(),
                                                         m_activeSnippetsEnd.at(group));
}

int SnippetsCollection::totalSnippets(Snippet::Group group) const
{
    return m_snippets.at(group).size();
}

void SnippetsCollection::clear()
{
    for (Snippet::Group group = Snippet::Cpp; group < Snippet::GroupSize; ++group) {
        m_snippets[group].clear();
        m_activeSnippetsEnd[group] = m_snippets[group].end();
    }
}

void SnippetsCollection::updateActiveSnippetsEnd(Snippet::Group group)
{
    m_activeSnippetsEnd[group] = std::find_if(m_snippets[group].begin(),
                                              m_snippets[group].end(),
                                              removedSnippetPred);
}
